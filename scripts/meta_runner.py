#!/usr/bin/env python3
import argparse
import datetime
import glob
import subprocess
import shutil
import os
import random
import re
import signal
import statistics
import sys
import time
import traceback
import yaml

import pdb

###############################################################################
# Argument parsing
###############################################################################

parser = argparse.ArgumentParser(description = "isl metamorphic testing runner")
parser.add_argument("mode", choices=["bounded", "coverage", "continuous", "targeted"],
    help = "Define the mode in which to run the testing.")
parser.add_argument("--seed-max", type=int, default=1000,
    help = "[bounded] Set the max seed (default 1000)")
parser.add_argument("--seed-min", type=int, default=0,
    help = "[bounded] Set the starting seed (default 0)")
parser.add_argument("--config-file", type=str,
    help = "Overwrite default config file to use.")
parser.add_argument("--lib-path", type=str,
    help = "Overwrite library path specified in config file")
parser.add_argument("--timeout", type=int, default=60,
    help = "The amount of time (in seconds) to run each test before giving up.")
parser.add_argument("--append-id", action='store_true',
    help = "If set, the produced file names will contain special IDs meant to\
        distinguish between various runs.")
args = parser.parse_args()

###############################################################################
# Set working paths and constants
###############################################################################

if not args.config_file:
    config_file_path = os.path.dirname(os.path.abspath(__file__))
    config_file_path += "/../config_files/config_isl.yaml"
else:
    config_file_path = args.config_file
    if not os.path.exists(config_file_path):
        print("Could not find given config file path: {}".format(
            config_file_path))
        exit(1)
with open(config_file_path, 'r') as config_file_fd:
    config_file = yaml.load(config_file_fd)
working_dir = config_file["working_dir"]
assert working_dir[-1] == os.sep
os.chdir(working_dir)

runner_config_data = config_file["meta_runner"]
# Path setup
test_emitter_path = runner_config_data["test_emitter_path"]
lib_path = runner_config_data["lib_path"]
lib_build_dir = runner_config_data["lib_build_dir"]
test_compile_dir = runner_config_data["test_compile_dir"]
# Test runtime setup
test_compile_bin = runner_config_data["test_compile_bin"]
test_source_path = runner_config_data["test_source_path"]
test_run_path = runner_config_data["test_run_path"]
# Output setup
output_folder = runner_config_data["output_folder"]
if not output_folder[-1] == os.sep:
    output_folder += os.sep
log_file = output_folder + runner_config_data["log_file_path"]
stat_log_file = output_folder + runner_config_data["stat_log_file_path"]
output_tests_folder = output_folder + runner_config_data["output_tests_folder"]
coverage_output_file = output_folder + runner_config_data["coverage_output_file"]

###############################################################################
# Helper functions
###############################################################################

def generate_test(seed, timeout, input_file_path = None):
    seed = str(seed)
    timeout = str(timeout)
    global test_count
    test_count += 1
    generator_cmd = [test_emitter_path, "-s", seed, "-o", test_source_path]
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
        # Path below is hack
        compile_cmd = [test_compile_bin, test_source_path.rsplit("/", 1)[1]]
        # print("CMD is " + " ".join(compile_cmd))
        compile_proc = subprocess.run(compile_cmd, check=True, cwd=test_compile_dir,
            stdout=subprocess.DEVNULL)
        return True
    except subprocess.CalledProcessError:
        log_writer.write("!!! Compilation Failure\n")
        return False

def append_id_to_string(string, run_id):
    id_string = "_" + str(run_id)
    if string[-1] == '/':
        return str(id_string + "/").join(string.rsplit('/', 1))
    if '/' in string:
        if '.' in string.rsplit('/', 1)[1]:
            return str(id_string + ".").join(string.rsplit('.', 1))
    return string + id_string

def execute_test(timeout, test_run_path):
    timeout = str(timeout)
    test_cmd = ["timeout", timeout, test_run_path]
    start_time = time.time()
    # print("CMD is " + " ".join(test_cmd))
    test_proc = subprocess.Popen(test_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    out, err = test_proc.communicate()
    check_stats(err)
    if test_proc.returncode != 0:
        log_writer.write("!!! Execution fail\n")
        if test_proc.returncode == 124:
            global timeout_count
            timeout_count += 1
        else:
            log_writer.write("Non-timeout\n")
        log_writer.write("RETURNCODE: " + str(test_proc.returncode) + "\n")
        log_writer.write("RUNTIME: " + str(time.time() - start_time) + "\n")
        log_writer.write("STDOUT:\n" + out + "\n")
        log_writer.write("STDERR:\n" + err + "\n")
    return test_proc.returncode == 0

def check_single_stat(regex, input_str):
    result_list = []
    for match in regex.finditer(input_str):
        result_list.append(int(match.group(0).split("= ")[1]))
    return result_list

def check_stats(err):
    # Global declarations
    global set_empty_regex
    global dim_set_regex
    global dim_set_list
    global dim_param_regex
    global dim_param_list
    global n_basic_set_regex
    global n_basic_set_list
    global n_constraint_regex
    global n_constraint_list
    # pdb.set_trace()
    for set_empty_match in set_empty_regex.finditer(err):
        global set_empty_count
        if "true" in set_empty_match.group(0):
            set_empty_count += 1
    dim_set_list.extend(check_single_stat(dim_set_regex, err))
    dim_param_list.extend(check_single_stat(dim_param_regex, err))
    n_basic_set_list.extend(check_single_stat(n_basic_set_regex, err))
    n_constraint_list.extend(check_single_stat(n_constraint_regex, err))

def write_single_stat(writer, stat_name, stat_data):
    writer.write("\t* {} mean: {}\n".format(stat_name, statistics.mean(stat_data)))
    writer.write("\t* {} median: {}\n".format(stat_name, statistics.median(stat_data)))

def write_stats():
    with open(stat_log_file, 'a') as stat_log_writer:
        stat_log_writer.write("END TIME: " + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S") + "\n")
        stat_log_writer.write("Statistics:\n")
        # Set empty stats
        stat_log_writer.write("\t* Empty sets: {} of {} ({}%)\n".format(
            set_empty_count, test_count, set_empty_count * 100 / test_count))
        # Timeout stats
        stat_log_writer.write("\t* Timeouts: {} of {} ({}%)\n".format(
            timeout_count, test_count, timeout_count * 100 / test_count))
        # Dim set stats
        if dim_set_list:
            write_single_stat(stat_log_writer, "Dim set", dim_set_list)
        # Dim param stats
        if dim_param_list:
            write_single_stat(stat_log_writer, "Dim param", dim_param_list)
        # N basic set stats
        if n_basic_set_list:
            write_single_stat(stat_log_writer, "N basic set", n_basic_set_list)
        # N constraint stats
        if n_constraint_list:
            write_single_stat(stat_log_writer, "N constraint", n_constraint_list)

def write_version_id(writer, path, id_name):
    try:
        if not path[-1] == os.sep:
            path += os.sep
        path += ".git"
        id_cmd = ["git", "--git-dir", path, "log", "--format=\"%H\"", "-n", "1"]
        id_proc = subprocess.run(id_cmd, check=True, encoding="utf-8",
            capture_output=True)
        writer.write("{} VERSION: {}".format(id_name, id_proc.stdout))
    except subprocess.CalledProcessError:
        pass

def get_coverage():
    coverage_cmd = ["gcovr", "-s", "-r", "."]
    try:
        coverage_proc = subprocess.run(coverage_cmd, check=True,
            capture_output=True, encoding="utf-8", cwd=lib_build_dir)
        return coverage_proc.stdout,coverage_proc.stderr
    except subprocess.CalledProcessError:
        return ("", "Error running gcovr!")
    except FileNotFoundError:
        return ("", traceback.format_exc())

def int_handler(sig, frame):
    print("Received SIGINT, dumping logged data and stopping...")
    finalize_experiments()
    exit(0)

def finalize_experiments():
    cov_stdout,cov_stderr=("", "Did not run")
    if lib_build_dir:
        assert os.path.exists(lib_build_dir)
        cov_stdout,cov_stderr = get_coverage()
    with open(coverage_output_file, 'w') as coverage_writer:
        coverage_writer.write("=== STDOUT\n")
        coverage_writer.write(cov_stdout)
        coverage_writer.write("=== STDERR\n")
        coverage_writer.write(cov_stderr)
    write_stats()
    log_writer.close()

###############################################################################
# Testing mode functions
###############################################################################

def bounded_testing(seed_min, seed_max):
    for seed in range(seed_min, seed_max):
        date_time = datetime.datetime.now().strftime("[%Y-%m-%d %H:%M:%S]")
        log_writer.write(80 * "=" + "\n")
        log_writer.write("SEED: " + str(seed) + "\n")
        print(date_time + " Running seed " + str(seed), end='\r')
        global test_source_path
        if not generate_test(seed, args.timeout):
            continue
        if not compile_test(test_compile_bin, test_compile_dir):
            shutil.copy(test_source_path, output_tests_folder + "/test_compile_" + str(seed) + ".cpp")
            continue
        if not execute_test(args.timeout, test_run_path):
            shutil.copy(test_source_path, output_tests_folder + "/test_run_" + str(seed) + ".cpp")
        log_writer.flush()

# def coverage_testing(coverage_target):
    # curr_coverage = 0
    # seed = 0
    # if (os.path.exists(coverage_output_dir)):
        # shutil.rmtree(coverage_output_dir)
    # os.mkdir(coverage_output_dir)
    # if (os.path.exists(coverage_data_file)):
        # os.remove(coverage_data_file)
    # gather_coverage_files()
    # while (curr_coverage < coverage_target and seed < 50):
        # print("=== Running seed " + str(seed), end='\r')
        # generate_test(seed, args.timeout, test_emitter_path)
        # compile_test(test_compile_bin, test_compile_dir)
        # execute_test(args.timeout, test_run_path)
        # new_coverage = get_coverage()
        # if new_coverage > curr_coverage:
            # shutil.move(test_run_path + ".cpp", coverage_output_dir + "test_" + str(seed) + ".cpp")
            # curr_coverage = new_coverage
            # print("New coverage at " + str(curr_coverage))
        # seed += 1
    # gather_coverage_files()

# TODO: log generating command, log time to execute, timeout y/n, any other thing?
def continuous_testing():
    while True:
        date_time = datetime.datetime.now().strftime("[%Y-%m-%d %H:%M:%S]")
        seed = random.randint(0, sys.maxsize)
        log_writer.write(80 * "=" + "\n")
        log_writer.write("SEED: " + str(seed) + "\n")
        print(date_time + " Running seed " + str(seed), end='\r')
        if not generate_test(seed, args.timeout):
            continue
        if not compile_test(test_compile_bin, test_compile_dir):
            shutil.copy(test_source_path, output_tests_folder + "/test_compile_"\
                + str(seed) + ".cpp")
            continue
        if not execute_test(args.timeout, test_run_path):
            shutil.copy(test_source_path, output_tests_folder + "/test_run_"\
                + str(seed) + ".cpp")
        log_writer.flush()

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
        if not generate_test(seed, args.timeout, test_emitter_path, input_sets_temp):
            continue
        if not compile_test(test_compile_bin, test_compile_dir):
            shutil.copy(test_source_path, output_tests_folder + "/test_compile_" + str(input_cnt) + ".cpp")
            continue
        if not execute_test(args.timeout, test_run_path):
            shutil.copy(test_source_path, output_tests_folder + "/test_run_" + str(input_cnt) + ".cpp")

###############################################################################
# Main entry point
###############################################################################

signal.signal(signal.SIGINT, int_handler)

# random.seed(42)
internal_seed = random.randrange(sys.maxsize)
random.seed(internal_seed)
if args.append_id:
    log_file = append_id_to_string(log_file, internal_seed)
    stat_log_file = append_id_to_string(stat_log_file, internal_seed)
    output_tests_folder = append_id_to_string(output_tests_folder, internal_seed)
    test_source_path = append_id_to_string(test_source_path, internal_seed)
    test_run_path = append_id_to_string(test_run_path, internal_seed)
    coverage_output_file = append_id_to_string(coverage_output_file, internal_seed)

if not os.path.exists(output_folder):
    os.mkdir(output_folder)

if os.path.exists(output_tests_folder):
    print("Found existing output folder {}, deleting...".format(output_tests_folder))
    shutil.rmtree(output_tests_folder)
os.mkdir(output_tests_folder)

# if lib_build_dir:
    # prev_cov_files = glob.glob(lib_build_dir + "/**/*.gcda")
    # for prev_cov_file in prev_cov_files:
        # print("Removing " + prev_cov_file)
        # # os.remove(prev_cov_file)
    # # exit(1)

if args.lib_path:
    lib_path = args.lib_path
if not os.path.exists(lib_path):
    print("Could not find given lib path " + lib_path)
    exit(1)
os.environ["LD_LIBRARY_PATH"] = lib_path

log_writer = open(log_file, 'w')

log_writer.write("START TIME: " + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
log_writer.write("\n")

test_count = 0
dim_set_list = []
dim_param_list = []
set_empty_count = 0
timeout_count = 0
n_basic_set_list = []
n_constraint_list = []
dim_set_regex = re.compile("^DIM SET = [0-9]+$", re.M)
dim_param_regex = re.compile("^DIM PARAM = [0-9]+$", re.M)
set_empty_regex = re.compile("^SET EMPTY = (true|false)$", re.M)
n_basic_set_regex = re.compile("^N BASIC SET = [0-9]+$", re.M)
n_constraint_regex = re.compile("^N CONSTRAINTS = [0-9]+$", re.M)

with open(stat_log_file, 'w') as stat_log_writer:
    stat_log_writer.write("MODE: " + args.mode + "\n")
    stat_log_writer.write("INTERNAL SEED: " + str(internal_seed) + "\n")
    stat_log_writer.write("TIMEOUT: " + str(args.timeout) + "\n")
    stat_log_writer.write("FUZZER_MODE: " + args.mode + "\n")
    write_version_id(stat_log_writer, working_dir, "METALIB")
    write_version_id(stat_log_writer, lib_build_dir, "LIB")

if args.mode == "bounded":
    bounded_testing(args.seed_min, args.seed_max)
elif args.mode == "coverage":
    assert False
    coverage_testing(coverage_target)
elif args.mode == "continuous":
    continuous_testing()
elif args.mode == "targeted":
    assert False
    targeted_testing()

finalize_experiments()

exit(0)
