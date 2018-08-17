#!/usr/bin/env python
import argparse
import datetime
import subprocess
import shutil
import os
import random
import re
import statistics
import sys
import time
import yaml

###############################################################################
# Set working paths and constants
###############################################################################

config_file_path = os.path.dirname(os.path.abspath(__file__))
config_file_path += "/../config_files/config.yaml"
with open(config_file_path, 'r') as config_file_fd:
    config_file = yaml.load(config_file_fd)
os.chdir(config_file["working_dir"])

runner_config_file = config_file["meta_runner"]
isl_tester_path = runner_config_file["isl_tester_path"]
test_compile_dir = runner_config_file["test_compile_dir"]
test_compile_bin = runner_config_file["test_compile_bin"]
test_source_path = runner_config_file["test_source_path"]
test_run_path = runner_config_file["test_run_path"]
log_file = runner_config_file["log_file_path"]
output_tests_folder = runner_config_file["output_tests_folder"]
default_timeout = runner_config_file["default_timeout"]

coverage_output_dir = "./out/coverage/"
input_sets_file = "./input_tests/input_sets_autotuner"
input_sets_temp = os.path.abspath("./input_tests/input_sets_temp")
coverage_source_file = "/home/sentenced/Documents/Internships/2018_ETH/isl_contrib/isl/isl_coalesce.c"
coverage_notes_file = "/home/sentenced/Documents/Internships/2018_ETH/isl_contrib/isl/.libs/isl_coalesce.gcno"
coverage_data_file = "/home/sentenced/Documents/Internships/2018_ETH/isl_contrib/isl/.libs/isl_coalesce.gcda"
coverage_target = 40

###############################################################################
# Argument parsing
###############################################################################

parser = argparse.ArgumentParser(description = "isl metamorphic testing runner")
parser.add_argument("mode", choices=["bounded", "coverage", "continuous", "targeted"],
    help = "Define the mode in which to run the testing.")
# parser.add_argument("tester_mode", choices=["SET_META_STR", "SET_META_API", "SET_META_NEW"],
    # help = "Define the mode used to generate inputs, in case of random inputs.")
parser.add_argument("--seed-max", type=int, default=1000,
    help = "[bounded] Set the max seed (default 1000)")
parser.add_argument("--seed-min", type=int, default=0,
    help = "[bounded] Set the starting seed (default 0)")
parser.add_argument("--output-log", type=str,
    help = "Overwrite output log location.")
parser.add_argument("--timeout", type=int, default=default_timeout,
    help = "The amount of time (in seconds) to run each test before giving up.")
args = parser.parse_args()

###############################################################################
# Helper functions
###############################################################################

def generate_test(seed, timeout, isl_tester_path, input_file_path = None):
    seed = str(seed)
    timeout = str(timeout)
    global test_count
    test_count += 1
    generator_cmd = [isl_tester_path, "-s", seed]
    # generator_cmd = [isl_tester_path, "-m", "SET_META", "-s", seed]
    if input_file_path:
        generator_cmd.extend(["--input-sets-file", input_file_path])
    # print("CMD is " + " ".join(generator_cmd))
    generator_proc = subprocess.Popen(generator_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    out, err = generator_proc.communicate()
    log_writer.write("CMD:\n" + " ".join(generator_cmd) + "\n")
    if generator_proc.returncode != 0:
        print("Return code: %s\n" % (generator_proc.returncode))
        print("%s \n%s\n" % (out, err))
        log_writer.write("!!! Generation failure\n")
        log_writer.write("RETURNCODE: " + str(generator_proc.returncode) + "\n")
        log_writer.write("STDOUT:\n" + out + "\n")
        log_writer.write("STDERR:\n" + err + "\n")
    return generator_proc.returncode == 0

def compile_test(test_compile_bin, test_compile_dir):
    try:
        compile_cmd = [test_compile_bin]
        compile_proc = subprocess.run(compile_cmd, check=True, cwd=test_compile_dir)
        return True
    except subprocess.CalledProcessError:
        log_writer.write("!!! Compilation Failure\n")
        return False

def execute_test(timeout, test_run_path):
    timeout = str(timeout)
    test_cmd = ["timeout", timeout, test_run_path]
    start_time = time.time()
    test_proc = subprocess.Popen(test_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    out, err = test_proc.communicate()
    check_stats(err)
    if test_proc.returncode != 0:
        log_writer.write("!!! Execution fail\n")
        log_writer.write("RETURNCODE: " + str(test_proc.returncode) + "\n")
        log_writer.write("RUNTIME: " + str(time.time() - start_time) + "\n")
        log_writer.write("STDOUT:\n" + out + "\n")
        log_writer.write("STDERR:\n" + err + "\n")
    return test_proc.returncode == 0

def check_stats(err):
    # Empty set check
    global set_empty_regex
    set_empty_match = set_empty_regex.search(err)
    if set_empty_match:
        global set_empty_count
        if "true" in set_empty_match.group(0):
            set_empty_count += 1
    # Set dim recording
    global dim_set_regex
    dim_set_match = dim_set_regex.search(err)
    if dim_set_match:
        global dim_set_list
        dim_set_list.append(int(dim_set_match.group(0).split("= ")[1]))
    # Set param recording
    global dim_param_regex
    dim_param_match = dim_param_regex.search(err)
    if dim_param_match:
        global dim_param_list
        dim_param_list.append(int(dim_param_match.group(0).split("= ")[1]))
    # N basic_set recording
    global n_basic_set_regex
    n_basic_set_match = n_basic_set_regex.search(err)
    if n_basic_set_match:
        global n_basic_set_list
        n_basic_set_list.append(int(n_basic_set_match.group(0).split("= ")[1]))
    # N constraint recording
    global n_constraint_regex
    n_constraint_match = n_constraint_regex.search(err)
    if n_constraint_match:
        global n_constraint_list
        n_constraint_list.append(int(n_constraint_match.group(0).split("= ")[1]))

def gather_coverage_files():
    shutil.copy(coverage_source_file, coverage_output_dir)
    shutil.copy(coverage_notes_file, coverage_output_dir)

def get_coverage():
    shutil.copy(coverage_data_file, coverage_output_dir)
    cmd = ["gcov", coverage_output_dir + "isl_coalesce"]
    gcov_output = subprocess.check_output(cmd, encoding="UTF-8", stderr = subprocess.DEVNULL)
    gcov_output = [x for x in gcov_output.split("\n\n") if "coalesce" in x]
    gcov_output = [x for x in gcov_output[0].split("\n") if "Lines executed" in x]
    gcov_output = gcov_output[0].split(":")[1].split("%")[0]
    return float(gcov_output)

###############################################################################
# Testing mode functions
###############################################################################

def bounded_testing(seed_min, seed_max):
    for seed in range(seed_min, seed_max):
        date_time = datetime.datetime.now().strftime("[%Y-%m-%d %H:%M:%S]")
        log_writer.write(80 * "=" + "\n")
        log_writer.write("SEED: " + str(seed) + "\n")
        print(date_time + " Running seed " + str(seed), end='\r')
        if not generate_test(seed, args.timeout, isl_tester_path):
            continue
        if not compile_test(test_compile_bin, test_compile_dir):
            shutil.copy(test_source_path, output_tests_folder + "/test_compile_" + str(seed) + ".cpp")
            continue
        if not execute_test(args.timeout, test_run_path):
            shutil.copy(test_source_path, output_tests_folder + "/test_run_" + str(seed) + ".cpp")
        log_writer.flush()

def coverage_testing(coverage_target):
    curr_coverage = 0
    seed = 0
    if (os.path.exists(coverage_output_dir)):
        shutil.rmtree(coverage_output_dir)
    os.mkdir(coverage_output_dir)
    if (os.path.exists(coverage_data_file)):
        os.remove(coverage_data_file)
    gather_coverage_files()
    while (curr_coverage < coverage_target and seed < 50):
        print("=== Running seed " + str(seed), end='\r')
        generate_test(seed, args.timeout, isl_tester_path)
        compile_test(test_compile_bin, test_compile_dir)
        execute_test(args.timeout, test_run_path)
        new_coverage = get_coverage()
        if new_coverage > curr_coverage:
            shutil.move(test_run_path + ".cpp", coverage_output_dir + "test_" + str(seed) + ".cpp")
            curr_coverage = new_coverage
            print("New coverage at " + str(curr_coverage))
        seed += 1
    gather_coverage_files()

# TODO: log generating command, log time to execute, timeout y/n, any other thing?
def continuous_testing():
    seed = 0
    while True:
        date_time = datetime.datetime.now().strftime("[%Y-%m-%d %H:%M:%S]")
        log_writer.write(80 * "=" + "\n")
        log_writer.write("SEED: " + str(seed))
        print(date_time + " Running seed " + str(seed), end='\r')
        generate_test(seed, args.timeout, isl_tester_path)
        compile_test(test_compile_bin, test_compile_dir)
        execute_test(args.timeout, test_run_path)
        seed += 1

def targeted_testing():
    max_tests_per_set = 10
    input_sets = []
    with open(input_sets_file, 'r') as input_reader:
        for line in input_reader:
            input_sets.append(line)
    for input_cnt,input_set in enumerate(input_sets):
        with open(input_sets_temp, 'w') as temp_writer:
            temp_writer.write(input_set)
        log_writer.write(80 * "=" + "\n")
        log_writer.write("SET: " + input_set)
        seed = random.randint(0, 2 ** 31)
        date_time = datetime.datetime.now().strftime("[%Y-%m-%d %H:%M:%S]")
        print("%s Running set %d of %d\n"
            % (date_time, input_cnt + 1, len(input_sets)),
                end='\r')
        if not generate_test(seed, args.timeout, isl_tester_path, input_sets_temp):
            continue
        if not compile_test(test_compile_bin, test_compile_dir):
            shutil.copy(test_source_path, output_tests_folder + "/test_compile_" + str(input_cnt) + ".cpp")
            continue
        if not execute_test(args.timeout, test_run_path):
            shutil.copy(test_source_path, output_tests_folder + "/test_run_" + str(input_cnt) + ".cpp")

###############################################################################
# Main entry point
###############################################################################

if (os.path.exists(output_tests_folder)):
    shutil.rmtree(output_tests_folder)
os.mkdir(output_tests_folder)

if args.output_log:
    log_writer = open(args.output_log, 'w')
else:
    log_writer = open(log_file, 'w')

log_writer.write("TIMEOUT: " + str(args.timeout) + "\n")
log_writer.write("MODE: " + args.mode + "\n")
# log_writer.write("FUZZER_MODE: " + args.tester_mode + "\n")
log_writer.write("START TIME: " + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
log_writer.write("\n")

test_count = 0
dim_set_list = []
dim_param_list = []
set_empty_count = 0
n_basic_set_list = []
n_constraint_list = []
dim_set_regex = re.compile("DIM SET = [0-9]+")
dim_param_regex = re.compile("DIM PARAM = [0-9]+")
set_empty_regex = re.compile("SET EMPTY = (true|false)")
n_basic_set_regex = re.compile("N BASIC SET = [0-9]+")
n_constraint_regex = re.compile("N CONSTRAINTS = [0-9]+")

random.seed(42)

if args.mode == "bounded":
    bounded_testing(args.seed_min, args.seed_max)
elif args.mode == "coverage":
    coverage_testing(coverage_target)
elif args.mode == "continuous":
    continuous_testing()
elif args.mode == "targeted":
    targeted_testing()

log_writer.write("\n" + 80 * "=" + "\n")
log_writer.write("END TIME: " + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S") + "\n")
log_writer.write("Statistics:\n")
# Set empty stats
log_writer.write("\t* Empty sets: {} of {} ({}%)\n".format(
    set_empty_count, test_count, set_empty_count * 100 / test_count))
# Dim set stats
log_writer.write("\t* Dim set average: {}\n".format(
    statistics.mean(dim_set_list)))
log_writer.write("\t* Dim set median: {}\n".format(
    statistics.median(dim_set_list)))
# Dim param stats
log_writer.write("\t* Dim param average: {}\n".format(
    statistics.mean(dim_param_list)))
log_writer.write("\t* Dim param median: {}\n".format(
    statistics.median(dim_param_list)))
# N basic set stats
log_writer.write("\t* N basic set average: {}\n".format(
    statistics.mean(n_basic_set_list)))
log_writer.write("\t* N basic set median: {}\n".format(
    statistics.median(n_basic_set_list)))
# N constraint stats
log_writer.write("\t* N constraint average: {}\n".format(
    statistics.mean(n_constraint_list)))
log_writer.write("\t* N constraint median: {}\n".format(
    statistics.median(n_constraint_list)))
log_writer.close()
exit(0)
