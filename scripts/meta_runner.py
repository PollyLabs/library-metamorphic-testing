#!/usr/bin/env python
import argparse
import datetime
import subprocess
import shutil
import os
import random
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
test_compile_path = runner_config_file["test_compile_path"]
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

log_writer = open(log_file, 'w')

###############################################################################
# Argument parsing
###############################################################################

parser = argparse.ArgumentParser(description = "isl metamorphic testing runner")
parser.add_argument("mode", choices=["bounded", "coverage", "continuous", "targeted"],
    help = "Define the mode in which to run the testing.")
parser.add_argument("tester_mode", choices=["SET_META_STR", "SET_META_API"],
    help = "Define the mode used to generate inputs, in case of random inputs.")
parser.add_argument("--seed-max", type=int, default=1000,
    help = "[bounded] Set the max number of tests to run (seed starts at 0)")
parser.add_argument("--timeout", type=int, default=default_timeout,
    help = "The amount of time (in seconds) to run each test before giving up.")
args = parser.parse_args()

###############################################################################
# Helper functions
###############################################################################

def generate_test(seed, timeout, isl_tester_path, input_file_path = None):
    seed = str(seed)
    timeout = str(timeout)
    generator_cmd = [isl_tester_path, "-m", args.tester_mode, "-s", seed]
    # generator_cmd = [isl_tester_path, "-m", "SET_META", "-s", seed]
    if input_file_path:
        generator_cmd.extend(["--input-sets-file", input_file_path])
    # print("CMD is " + " ".join(generator_cmd))
    generator_proc = subprocess.Popen(generator_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    out, err = generator_proc.communicate()
    print(out)
    log_writer.write("CMD:\n" + " ".join(generator_cmd) + "\n")
    if generator_proc.returncode != 0:
        print("Return code: %s\n" % (generator_proc.returncode))
        print("%s \n%s\n" % (out, err))
        log_writer.write("!!! Generation failure\n")
        log_writer.write("RETURNCODE: " + str(generator_proc.returncode) + "\n")
        log_writer.write("STDOUT:\n" + out + "\n")
        log_writer.write("STDERR:\n" + err + "\n")
    return generator_proc.returncode == 0

def compile_test(test_compile_path, test_compile_dir):
    try:
        compile_cmd = [test_compile_path]
        compile_proc = subprocess.run(compile_cmd, shell=True, check=True, cwd=test_compile_dir)
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
    if test_proc.returncode != 0:
        log_writer.write("!!! Execution fail\n")
        log_writer.write("RETURNCODE: " + str(test_proc.returncode) + "\n")
        log_writer.write("RUNTIME: " + str(time.time() - start_time) + "\n")
        log_writer.write("STDOUT:\n" + out + "\n")
        log_writer.write("STDERR:\n" + err + "\n")
    return test_proc.returncode == 0

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

def bounded_testing(seed_max):
    for seed in range(0, seed_max):
        date_time = datetime.datetime.now().strftime("[%Y-%m-%d %H:%M:%S]")
        log_writer.write(80 * "=" + "\n")
        log_writer.write("SEED: " + str(seed))
        print(date_time + " Running seed " + str(seed), end='\r')
        if not generate_test(seed, args.timeout, isl_tester_path):
            continue
        if not compile_test(test_compile_path, test_compile_dir):
            shutil.copy(test_source_path, output_tests_folder + "/test_compile_" + str(input_cnt) + ".cpp")
            continue
        if not execute_test(args.timeout, test_run_path):
            shutil.copy(test_source_path, output_tests_folder + "/test_run_" + str(input_cnt) + ".cpp")

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
        compile_test(test_compile_path, test_compile_dir)
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
        compile_test(test_compile_path, test_compile_dir)
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
        if not compile_test(test_compile_path, test_compile_dir):
            shutil.copy(test_source_path, output_tests_folder + "/test_compile_" + str(input_cnt) + ".cpp")
            continue
        if not execute_test(args.timeout, test_run_path):
            shutil.copy(test_source_path, output_tests_folder + "/test_run_" + str(input_cnt) + ".cpp")

###############################################################################
# Main entry point
###############################################################################

log_writer.write("TIMEOUT: " + str(args.timeout) + "\n")
log_writer.write("MODE: " + args.mode + "\n")
log_writer.write("\n")

if (os.path.exists(output_tests_folder)):
    shutil.rmtree(output_tests_folder)
os.mkdir(output_tests_folder)

random.seed(42)

if args.mode == "bounded":
    bounded_testing(args.seed_max)
elif args.mode == "coverage":
    coverage_testing(coverage_target)
elif args.mode == "continuous":
    continuous_testing()
elif args.mode == "targeted":
    targeted_testing()
exit(0)
