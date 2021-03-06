#!/usr/bin/env python3
import argparse
import datetime
import glob
import io
import subprocess
import shutil
import os
import multiprocessing
import random
import re
import pathlib
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
parser.add_argument("--include-path", type=str,
    help = "Overwrite include path specified in config file")
parser.add_argument("--timeout", type=int,
    help = "The amount of time (in seconds) to run each test before giving up.")
parser.add_argument("--append-id", action='store_true',
    help = "If set, the produced file names will contain special IDs meant to\
        distinguish between various runs.")
parser.add_argument("--debug", action='store_true',
    help = "If set, produces additional runtime information for debugging\
    purposes.")
parser.add_argument("--max-par-proc", type=int, default=1,
    help = "The number of maximum jobs to issue in parallel (default: 1)")

###############################################################################
# Helper functions
###############################################################################

def print_debug(message, debug):
    if debug:
        print("*** {} - {}".format(message, datetime.datetime.now().strftime("%H:%M:%S")))

def set_path_env_var(env_var, value):
    for path_value in value.split(':'):
        if not os.path.exists(path_value):
            print("Could not find given path {} for env var {}.".format(path_value, env_var))
            exit(1)
    os.environ[env_var] = value

def generate_test(seed, test_id, runtime_data, log_data, par_data):
    print_debug("generate_test START", runtime_data["debug"])
    seed = str(seed)
    timeout = str(runtime_data["timeout"])
    par_data["stat_lock"].acquire()
    try:
        par_data["stats"]["test_count"] += 1
    finally:
        par_data["stat_lock"].release()
    generator_cmd = [runtime_data["test_emitter_path"], "-s", seed, "-o",
        runtime_data["test_source_path"], "-c", runtime_data["config_file_path"]]
    # print("CMD is " + " ".join(generator_cmd))
    generator_proc = subprocess.Popen(generator_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    out, err = generator_proc.communicate()
    log_data.write("CMD:\n" + " ".join(generator_cmd) + "\n")
    if generator_proc.returncode != 0:
        print("Return code: %s\n" % (generator_proc.returncode))
        print("%s \n%s\n" % (out, err))
        log_data.write("!!! Generation failure\n")
        log_data.write("RETURNCODE: " + str(generator_proc.returncode) + "\n")
        log_data.write("STDOUT:\n" + out + "\n")
        log_data.write("STDERR:\n" + err + "\n")
    print_debug("generate_test END", runtime_data["debug"])
    return generator_proc.returncode == 0

def compile_test(runtime_data, log_data):
    print_debug("compile_test START", runtime_data["debug"])
    try:
        # Path below is hack
        timeout_value = str(runtime_data["timeout"])
        compile_cmd = ["timeout", timeout_value,
            runtime_data["test_compile_bin"],
            runtime_data["test_source_path"].rsplit("/", 1)[1]]
        # print("CMD is " + " ".join(compile_cmd))
        compile_proc = subprocess.run(compile_cmd, check=True,
            cwd=runtime_data["test_compile_dir"], stdout=subprocess.DEVNULL)
        print_debug("compile_test END True", runtime_data["debug"])
        return True
    except subprocess.CalledProcessError:
        log_data.write("!!! Compilation Failure\n")
        print_debug("compile_test END False", runtime_data["debug"])
        return False

def append_id_to_string(string, run_id):
    id_string = "_" + str(run_id)
    if string[-1] == '/':
        return str(id_string + "/").join(string.rsplit('/', 1))
    if '/' in string:
        if '.' in string.rsplit('/', 1)[1]:
            return str(id_string + ".").join(string.rsplit('.', 1))
    return string + id_string

def execute_test(runtime_data, log_data, par_data):
    print_debug("execute_test START", runtime_data["debug"])
    timeout_value = str(runtime_data["timeout"])
    test_cmd = ["timeout", timeout_value, runtime_data["test_run_path"]]
    # print("CMD is " + " ".join(test_cmd))
    print_debug("execute_test > subprocess.Popen START", runtime_data["debug"])
    test_proc = subprocess.Popen(test_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    print_debug("execute_test > subprocess.Popen END", runtime_data["debug"])
    print_debug("execute_test > subprocess.communicate START", runtime_data["debug"])
    start_time = time.time()
    out, err = test_proc.communicate()
    end_time = time.time()
    print_debug("execute_test > subprocess.communicate END", runtime_data["debug"])
    par_data["stat_lock"].acquire()
    print_debug("execute_test > update_stats START", runtime_data["debug"])
    try:
        par_data["stats"] = update_stats(err, par_data["stats"], log_data)
    finally:
        par_data["stat_lock"].release()
    print_debug("execute_test > update_stats END", runtime_data["debug"])
    log_data.write("SIZE: " + str(os.path.getsize(runtime_data["test_run_path"])) + "\n")
    log_data.write("RUNTIME: " + str(end_time - start_time) + "\n")
    if test_proc.returncode != 0:
        log_data.write("!!! Execution fail\n")
        par_data["stat_lock"].acquire()
        try:
            if test_proc.returncode == 124:
                par_data["stats"]["timeout_count"] += 1
            else:
                par_data["stats"]["non_to_fail_count"] += 1
                log_data.write("Non-timeout\n")
        finally:
            par_data["stat_lock"].release()
        log_data.write("RETURNCODE: " + str(test_proc.returncode) + "\n")
        log_data.write("STDOUT:\n" + out + "\n")
        log_data.write("STDERR:\n" + err + "\n")
    print_debug("execute_test END", runtime_data["debug"])
    # print("*** execute_test RUNTIME - " + str(end_time - start_time))
    return test_proc.returncode == 0 or test_proc.returncode == 124

def check_single_list_stat(regex, input_str):
    result_list = []
    for match in regex.finditer(input_str):
        result_list.append(int(match.group(0).split("= ")[1]))
    return result_list

def check_single_count_stat(regex, input_str):
    return len(regex.findall(input_str))

def update_stats(err, stats, log_data):
    for key in stats.keys():
        if not isinstance(key, tuple):
            continue;
        if key[1] == "count":
            count_stat = check_single_count_stat(key[0], err)
            log_data.write("STAT {}: {}\n".format(key[2], count_stat))
            stats[key] += count_stat
        elif key[1] == "extend":
            list_stat = check_single_list_stat(key[0], err)
            log_data.write("STAT {}: {}\n".format(key[2], list_stat))
            stats[key] = stats[key] + list_stat
        else:
            assert(false)
    return stats

def write_single_stat(writer, stat_name, stat_data):
    writer.write("\t* {} mean: {}\n".format(stat_name, statistics.mean(stat_data)))
    writer.write("\t* {} median: {}\n".format(stat_name, statistics.median(stat_data)))

def write_stats(stat_log_file, stats):
    # pdb.set_trace()
    with open(stat_log_file, 'w') as stat_log_writer:
        stat_log_writer.write(stats["preamble"].getvalue())
        stat_log_writer.write("END TIME: " + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S") + "\n")
        stat_log_writer.write("Statistics:\n")

        for k,v in stats.items():
            if isinstance(k, str):
                stat_log_writer.write("\t* {} : {}\n".format(k, v))
            elif isinstance(k, tuple):
                if k[1] == "count":
                    stat_log_writer.write("\t* {}: {}\n".format(k[2], v))
                elif k[1] == "extend":
                    if v:
                        write_single_stat(stat_log_writer, k[2], v)
                else:
                    print("Found {} stat log type".format(k[1]))
                    assert False
            else:
                assert False

def write_version_id(writer, path, id_name):
    try:
        if not path:
            return
        if not path[-1] == os.sep:
            path += os.sep
        path += ".git"
        id_cmd = ["git", "--git-dir", path, "log", "--format=\"%H\"", "-n", "1"]
        id_proc = subprocess.run(id_cmd, check=True, encoding="utf-8",
            capture_output=True)
        writer.write("{} VERSION: {}".format(id_name, id_proc.stdout))
    except subprocess.CalledProcessError:
        pass

def get_coverage(runtime_data):
    cwd = os.getcwd()
    os.chdir(runtime_data["lib_coverage_dir"])
    gcda_list = pathlib.Path('.').glob("./**/*.gcda")
    for gcda_file in gcda_list:
        dst_dir = cwd + "/" + runtime_data["coverage_output_dir"] + os.path.dirname(gcda_file)
        os.makedirs(dst_dir, exist_ok=True)
        try:
            shutil.copy(str(gcda_file), dst_dir)
            shutil.copy(str(gcda_file).replace(".gcda", ".gcno"), dst_dir)
        except FileNotFoundError:
            continue
    os.chdir(cwd)

def int_handler(sig, frame):
    print("Received SIGINT, dumping logged data and stopping...")
    for proc in multiprocessing.active_children():
        proc.kill()
        proc.join()
    # finalize_experiments()
    exit(0)

def finalize_experiments(runtime_data, par_data):
    # pdb.set_trace()
    cov_stdout,cov_stderr=("", "Did not run")
    if runtime_data["lib_coverage_dir"]:
        assert os.path.exists(runtime_data["lib_coverage_dir"])
        get_coverage(runtime_data)
    write_stats(runtime_data["stat_log_file"], par_data["stats"])

###############################################################################
# Testing mode functions
###############################################################################

def bounded_testing(seed_min, seed_max, test_id, runtime_data, par_data):
    runtime_data["test_source_path"] =\
        append_id_to_string(runtime_data["test_source_path"], test_id)
    runtime_data["test_run_path"] += "_" + str(test_id)
    last_test_count = 0
    for seed in range(seed_min, seed_max):
        log_data = io.StringIO()
        date_time = datetime.datetime.now().strftime("[%Y-%m-%d %H:%M:%S]")
        log_data.write(80 * "=" + "\n")
        log_data.write("SEED: " + str(seed) + "\n")
        end_char = '\r'
        if runtime_data["debug"]:
            end_char = '\n'
        print(date_time + " Running seed " + str(seed) + " on " + str(test_id),
            end= end_char)
        test_source_path = runtime_data["test_source_path"]
        output_tests_folder = runtime_data["output_tests_folder"]
        if not generate_test(seed, test_id, runtime_data, log_data, par_data):
            continue
        if not compile_test(runtime_data, log_data):
            shutil.copy(test_source_path, output_tests_folder + "/test_compile_"\
                + str(seed) + ".cpp")
            continue
        if not execute_test(runtime_data, log_data, par_data):
            shutil.copy(test_source_path, output_tests_folder + "/test_run_" + str(seed) + ".cpp")
        par_data["log_lock"].acquire()
        try:
            with open(par_data["log_file"], 'a') as log_writer:
                log_writer.write(log_data.getvalue())
                log_writer.flush()
        finally:
            par_data["log_lock"].release()
        log_data.close()
        if test_id == 0 and par_data["stats"]["test_count"] - last_test_count > 10:
            finalize_experiments(runtime_data, par_data)
            last_test_count = par_data["stats"]["test_count"]

def continuous_testing(test_id, runtime_data, par_data):
    runtime_data["test_source_path"] =\
        append_id_to_string(runtime_data["test_source_path"], test_id)
    runtime_data["test_run_path"] += "_" + str(test_id)
    last_test_count = 0
    while True:
        log_data = io.StringIO()
        date_time = datetime.datetime.now().strftime("[%Y-%m-%d %H:%M:%S]")
        seed = random.randint(0, sys.maxsize)
        log_data.write(80 * "=" + "\n")
        log_data.write("SEED: " + str(seed) + "\n")
        print(date_time + " Running seed " + str(seed) + " on " + str(test_id), end='\r')
        test_source_path = runtime_data["test_source_path"]
        output_tests_folder = runtime_data["output_tests_folder"]
        if not generate_test(seed, test_id, runtime_data, log_data, par_data):
            continue
        if not compile_test(runtime_data, log_data):
            shutil.copy(test_source_path, output_tests_folder + "/test_compile_"\
                + str(seed) + ".cpp")
            continue
        if not execute_test(runtime_data, log_data, par_data):
            shutil.copy(test_source_path, output_tests_folder + "/test_run_"\
                + str(seed) + ".cpp")
        par_data["log_lock"].acquire()
        try:
            with open(par_data["log_file"], 'a') as log_writer:
                log_writer.write(log_data.getvalue())
                log_writer.flush()
        finally:
            par_data["log_lock"].release()
        log_data.close()
        if test_id == 0 and par_data["stats"]["test_count"] - last_test_count > 10:
            finalize_experiments(runtime_data, par_data)
            last_test_count = par_data["stats"]["test_count"]

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
        if not generate_test(seed, timeout, test_emitter_path, input_sets_temp):
            continue
        if not compile_test(test_compile_bin, test_compile_dir):
            shutil.copy(test_source_path, output_tests_folder + "/test_compile_" + str(input_cnt) + ".cpp")
            continue
        if not execute_test(timeout, test_run_path):
            shutil.copy(test_source_path, output_tests_folder + "/test_run_" + str(input_cnt) + ".cpp")

###############################################################################
# Main entry point
###############################################################################

if __name__ == '__main__':
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
        config_file = yaml.load(config_file_fd, Loader=yaml.FullLoader)
    working_dir = config_file["working_dir"]
    assert working_dir[-1] == os.sep
    os.chdir(working_dir)

    runner_config_data = config_file["meta_runner"]
# Path setup
    test_emitter_path = runner_config_data["test_emitter_path"]
    lib_path = runner_config_data["lib_path"]
    include_path = runner_config_data["include_path"]
    lib_build_dir = runner_config_data["lib_build_dir"]
    if (runner_config_data["lib_coverage_dir"]):
        lib_coverage_dir = runner_config_data["lib_coverage_dir"]
    else:
        lib_coverage_dir = runner_config_data["lib_build_dir"]
    test_compile_dir = runner_config_data["test_compile_dir"]
# Test runtime setup
    test_compile_bin = runner_config_data["test_compile_bin"]
    test_source_path = runner_config_data["test_source_path"]
    test_run_path = runner_config_data["test_run_path"]
    if args.timeout:
        timeout = args.timeout
    else:
        timeout = runner_config_data["default_timeout"]

# Output setup
    output_folder = runner_config_data["output_folder"]
    if not output_folder[-1] == os.sep:
        output_folder += os.sep
    log_file = output_folder + runner_config_data["log_file_path"]
    stat_log_file = output_folder + runner_config_data["stat_log_file_path"]
    output_tests_folder = output_folder + runner_config_data["output_tests_folder"]
    coverage_output_dir = output_folder + runner_config_data["coverage_output_dir"]
    api_fuzzer_file_out = output_folder + config_file["api_fuzzer_file"]
    meta_test_file_out = output_folder + config_file["meta_test_file"]

# Signal handler setup
    signal.signal(signal.SIGINT, int_handler)

# Internal setup
    internal_seed = random.randrange(sys.maxsize)
    random.seed(internal_seed)
    if args.append_id:
        log_file = append_id_to_string(log_file, internal_seed)
        stat_log_file = append_id_to_string(stat_log_file, internal_seed)
        output_tests_folder = append_id_to_string(output_tests_folder, internal_seed)
        test_source_path = append_id_to_string(test_source_path, internal_seed)
        test_run_path = append_id_to_string(test_run_path, internal_seed)
        coverage_output_dir = append_id_to_string(coverage_output_dir, internal_seed)
        api_fuzzer_file_out = append_id_to_string(api_fuzzer_file_out, internal_seed)
        meta_test_file_out = append_id_to_string(meta_test_file_out, internal_seed)

    if not os.path.exists(output_folder):
        os.mkdir(output_folder)

    if os.path.exists(output_tests_folder):
        print("Found existing output folder {}, deleting...".format(output_tests_folder))
        shutil.rmtree(output_tests_folder)
    os.mkdir(output_tests_folder)

# Copy over specifications file
    os.makedirs(os.path.dirname(api_fuzzer_file_out), exist_ok=True)
    shutil.copy(config_file["api_fuzzer_file"], api_fuzzer_file_out)
    shutil.copy(config_file["meta_test_file"], meta_test_file_out)

    if args.lib_path:
        lib_path = args.lib_path
    set_path_env_var("LD_LIBRARY_PATH", lib_path)
    set_path_env_var("LIBRARY_PATH", lib_path)
    if args.include_path:
        include_path = args.include_path
    set_path_env_var("CPATH", include_path)

    dim_set_regex = re.compile("^DIM SET = [0-9]+$", re.M)
    dim_param_regex = re.compile("^DIM PARAM = [0-9]+$", re.M)
    set_empty_regex = re.compile("^SET EMPTY = true$", re.M)
    n_basic_set_regex = re.compile("^N BASIC SET = [0-9]+$", re.M)
    n_constraint_regex = re.compile("^N CONSTRAINTS = [0-9]+$", re.M)

    stat_data = io.StringIO()
    stat_data.write("MODE: " + args.mode + "\n")
    stat_data.write("INTERNAL SEED: " + str(internal_seed) + "\n")
    stat_data.write("TIMEOUT: " + str(timeout) + "\n")
    stat_data.write("FUZZER_MODE: " + args.mode + "\n")
    write_version_id(stat_data, working_dir, "METALIB")
    write_version_id(stat_data, lib_build_dir, "LIB")
    stat_data.write("START TIME: " + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
    stat_data.write("\n")


# Execution declarations
    ctx = multiprocessing.get_context("spawn")
    with ctx.Manager() as manager:
        log_lock = manager.Lock()
        stat_lock = manager.Lock()
        stats = manager.dict({
            "preamble": stat_data,
            "test_count": 0,
            "timeout_count": 0,
            "non_to_fail_count": 0,
            (set_empty_regex, "count", "Set empty"): 0,
            (dim_set_regex, "extend", "Dim set"): [],
            (dim_param_regex, "extend", "Dim param"): [],
            (n_basic_set_regex, "extend", "N basic set"): [],
            (n_constraint_regex, "extend", "N constraint"): [],
        })
        runtime_data = {
            "timeout": timeout,
            "config_file_path": config_file_path,
            "test_compile_bin": test_compile_bin,
            "test_compile_dir": test_compile_dir,
            "test_run_path": test_run_path,
            "test_source_path": test_source_path,
            "output_tests_folder": output_tests_folder,
            "test_emitter_path": test_emitter_path,
            "lib_coverage_dir": lib_coverage_dir,
            "lib_build_dir": lib_build_dir,
            "coverage_output_dir": coverage_output_dir,
            "stat_log_file": stat_log_file,
            "debug": args.debug,
        }
        par_data = manager.dict({
            "log_lock": log_lock,
            "stat_lock": stat_lock,
            "stats": stats,
            "log_file": log_file,
        })

        proc_count = args.max_par_proc
        if args.mode == "bounded":
            interval_count = int((args.seed_max - args.seed_min) / proc_count)
            range_args = []
            prev_seed = args.seed_min
            for i in range(0, proc_count):
                if i == proc_count - 1:
                    interval_count = args.seed_max - prev_seed
                range_args.append((prev_seed, prev_seed + interval_count, i, runtime_data, par_data))
                prev_seed += interval_count

            proc_list = []
            for i in range(0, len(range_args)):
                proc_list.append(ctx.Process(target=bounded_testing, args=range_args[i]))
                proc_list[-1].start()
            for proc in proc_list:
                proc.join()
        elif args.mode == "continuous":
            proc_args = []
            proc_list = []
            for i in range(0, proc_count):
                proc_list.append(ctx.Process(target=continuous_testing, \
                    args=(i, runtime_data, par_data)))
                proc_list[-1].start()
            for proc in proc_list:
                proc.join()
        elif args.mode == "targeted":
            assert False
            targeted_testing()

        finalize_experiments(runtime_data, par_data)

    exit(0)
