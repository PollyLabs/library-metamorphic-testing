#!/usr/bin/python
import subprocess
import shutil

isl_tester_path = "./bin/isl_tester"
test_compile_dir = "./out"
test_compile_path = "./compile.sh"
test_run_path = "./out/test"
test_save_dir = "./out/cov_tests/"
log_file = "./out/meta_test.log"
timeout = 30
coverage_file = "/home/sentenced/Documents/Internships/2018_ETH/isl_contrib/isl/.libs/isl_coalesce"
coverage_target = 80

log_writer = open(log_file, 'w')
seed_max = 10

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

def get_coverage():
    cmd = ["gcov", coverage_file]
    gcov_output = subprocess.check_output(cmd, encoding="UTF-8", stderr = subprocess.DEVNULL)
    gcov_output = [x for x in gcov_output.split("\n\n") if "coalesce" in x]
    gcov_output = [x for x in gcov_output[0].split("\n") if "Lines executed" in x]
    gcov_output = gcov_output[0].split(":")[1].split("%")[0]
    return float(gcov_output)

def coverage_testing(coverage_target):
    curr_coverage = 0
    seed = 0
    while (curr_coverage < coverage_target):
        print("=== Running seed " + str(seed), end='\r')
        generate_test(seed, timeout, isl_tester_path)
        compile_test(test_compile_path, test_compile_dir)
        execute_test(timeout, test_run_path)
        new_coverage = get_coverage()
        if new_coverage > curr_coverage:
            shutil.move("./out/test.cpp", "./out/cov_tests/test_" + str(seed) + ".cpp")
            curr_coverage = new_coverage
            print("New coverage at " + str(curr_coverage))
        seed += 1

def random_testing(seed_max):
    for seed in range(0, seed_max):
        log_writer.write("=== Testing seed " + str(seed) + "\n")
        print("== Run seed " + str(seed))
        generate_test(seed, timeout, isl_tester_path)
        compile_test(test_compile_path, test_compile_dir)
        execute_test(timeout, test_run_path)

coverage_testing(coverage_target)
