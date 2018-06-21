#!/usr/bin/python
import argparse
import subprocess
import shutil
import os

###############################################################################
# Set working paths and constants (move these to config file?)
###############################################################################

os.chdir("/home/sentenced/Documents/Internships/2018_ETH/work/sets")

isl_tester_path = "./bin/isl_tester"
test_compile_dir = "./out"
test_compile_path = "./compile.sh"
test_run_path = "./out/test"
coverage_output_dir = "./out/coverage/"
log_file = "./out/meta_test.log"
timeout = 30
coverage_source_file = "/home/sentenced/Documents/Internships/2018_ETH/isl_contrib/isl/isl_coalesce.c"
coverage_notes_file = "/home/sentenced/Documents/Internships/2018_ETH/isl_contrib/isl/.libs/isl_coalesce.gcno"
coverage_data_file = "/home/sentenced/Documents/Internships/2018_ETH/isl_contrib/isl/.libs/isl_coalesce.gcda"
coverage_target = 40

log_writer = open(log_file, 'w')
seed_max = 10

###############################################################################
# Argument parsing
###############################################################################

parser = argparse.ArgumentParser(description = "isl metamorphic testing runner")
parser.add_argument("mode", choices=["bounded", "coverage", "continuous"],
    help = "Define the mode in which to run the testing.")
args = parser.parse_args()

###############################################################################
# Helper functions
###############################################################################

def generate_test(seed, timeout, isl_tester_path):
    seed = str(seed)
    timeout = str(timeout)
    generator_cmd = ["timeout", timeout, isl_tester_path, "-m", "SET_META", "-s", seed]
    log_writer.write("- Starting generation\n")
    generator_proc = subprocess.Popen(generator_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    out, err = generator_proc.communicate()
    log_writer.write("STDOUT:\n" + out + "\n")
    log_writer.write("STDERR:\n" + err + "\n")
    if generator_proc.returncode != 0:
        return
    log_writer.write("- End generation\n")

def compile_test(test_compile_path, test_compile_dir):
    try:
        log_writer.write("- Starting compile\n")
        compile_cmd = [test_compile_path]
        compile_proc = subprocess.run(compile_cmd, shell=True, check=True, cwd=test_compile_dir)
        log_writer.write("- End compile\n")
    except subprocess.CalledProcessError:
        print("Failed compiling.")

def execute_test(timeout, test_run_path):
    timeout = str(timeout)
    test_cmd = ["timeout", timeout, test_run_path]
    log_writer.write("- Starting test execution\n")
    test_proc = subprocess.Popen(test_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    out, err = test_proc.communicate()
    log_writer.write("STDOUT:\n" + out + "\n")
    log_writer.write("STDERR:\n" + err + "\n")
    if test_proc.returncode != 0:
        return
    log_writer.write("- Ending test execution\n")

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
        log_writer.write("=== Testing seed " + str(seed) + "\n")
        print("== Run seed " + str(seed))
        generate_test(seed, timeout, isl_tester_path)
        compile_test(test_compile_path, test_compile_dir)
        execute_test(timeout, test_run_path)

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
        generate_test(seed, timeout, isl_tester_path)
        compile_test(test_compile_path, test_compile_dir)
        execute_test(timeout, test_run_path)
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
        print("=== Running seed " + str(seed), end='\r')
        generate(test(seed, timeout, isl_tester_path)
        compile_test(test_compile_path, test_compile_dir)
        execute_test(timeout, test_run_path)
        seed += 1

###############################################################################
# Main entry point
###############################################################################

if args.mode == "bounded":
    bounded_testing(seed_max)
elif args.mode == "coverage":
    coverage_testing(coverage_target)
elif args.mode == "continuous":
    raise Exception("Not implemented")
exit(0)
