#!/usr/bin/env python3.7
import argparse
import glob
import subprocess
import pdb
import os
import sys
import time

from threading import Timer

import perf_graph_plot as pfgraph
import perf_stats as pfstats

import matplotlib
import matplotlib.rcsetup as rcs

################################################################################
# Argument parsing
################################################################################

parser = argparse.ArgumentParser()
parser.add_argument("--target-dir", type=str, default='./tests',
    help = "Folder containing files to execute and record.")
parser.add_argument("--target-regex", type=str, default='**/*',
    help = "glob regex to identify target files from target directory.")
parser.add_argument("--cpu", type=int, default=1,
    help = "Which cpu core to pin the process to; core id passed to `taskset`.")
parser.add_argument("--timeout", type=int, default=60,
    help = "Timeout for each individual test execution.")
parser.add_argument("--repeat-count", type=int, default=200,
    help = "Number of times to execute one test.")
parser.add_argument("--warmup-ratio", type=float, default=10.0,
    help = "Percent of total number of iterations to perform as warmup.")
parser.add_argument("--bin-ratio", type=float, default=5.0,
    help = "Number of total bins for histogram, as percent of total tests.")
parser.add_argument("--input-file", type=argparse.FileType('r'), default=None,
    help = "A file with runtime data to be used to generate statistics instead\
    of generating them from scratch.")
parser.add_argument("--debug", action='store_true', help = "Emit debug messages.")
parser.add_argument("--plots", action='store_true',
    help = "Whether to generate plots or not.")
parser.add_argument("--terminal", action='store_true',
    help = "Wether to emit statistics to terminal (if unset, will emit\
    statistics to default output file.")
args = parser.parse_args()

def debug_log(msg):
    if args.debug:
        print("DEBUG:" + msg)

def emit_handler(msg, output_file):
    if args.terminal:
        print(msg)
    else:
        with open(output_file, 'w') as emit_file:
            emit_file.write(msg)

################################################################################
# Setup
################################################################################


test_dir = args.target_dir
if test_dir[-1] != os.path.sep:
    test_dir += os.path.sep
input_glob = test_dir + args.target_regex
file_run = [x for x in glob.glob(input_glob)]
#for x in file_run:
#   print(x)
#sys.exit(1)
file_run_times = {}
file_warmup_times = {}

warmup_count = int(args.repeat_count * args.warmup_ratio / 100)
bin_count = int(args.repeat_count * args.bin_ratio / 100)
if bin_count == 0:
    bin_count = int(max(1, args.repeat_count / 2))

out_folder = "./perf_smt/graphs"
out_count = 0
while os.path.exists(f"{out_folder}_{out_count:03d}"):
    out_count += 1
out_folder = f"{out_folder}_{out_count:03d}"
os.makedirs(out_folder)

# print(rcs.all_backends)
# exit()
matplotlib.use("Agg")

if args.input_file:
    file_run_times["input"] = [float(line.rstrip()) for line in args.input_file.readlines()]
else:
    for test_file in file_run:
        file_run_times[test_file] = []
        file_warmup_times[test_file] = []
        fig_name_out = out_folder + "/" + test_file
        fig_name_out = fig_name_out.replace("..", ".")
        try:
            os.makedirs(os.path.split(fig_name_out)[0])
        except FileExistsError:
            pass
        cpu_pin_cmd = ["taskset", str(args.cpu), test_file]
        # Run training for hardware
        for i in range(0, warmup_count):
            print("Training " + test_file + " iteration " + str(i), end="\r")
            start_time = time.time()
            test_proc = subprocess.Popen(cpu_pin_cmd, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            timer = Timer(args.timeout, test_proc.kill)
            try:
                timer.start()
                outs, errs = test_proc.communicate()
            finally:
                end_time = time.time()
                timer.cancel()
                if test_proc.returncode != 0:
                    file_warmup_times[test_file].append(-1)
                else:
                    file_warmup_times[test_file].append(end_time - start_time)
        print()
        with open(fig_name_out + ".warmup.data", 'w') as warmup_fd:
            warmup_fd.write("\n".join(map(str, file_warmup_times[test_file])))
        with open(fig_name_out + ".warmup.log", 'w') as log_fd:
            log_fd.write(f"Warmup {fig_name_out} iteration {str(i + 1)} of {warmup_count}")
            log_fd.write(f"STDOUT:\n{outs}")
            log_fd.write(f"STDERR:\n{errs}")
            log_fd.write(80 * '=')
        for i in range(0, args.repeat_count):
            print("Running " + test_file + " iteration " + str(i), end="\r")
            start_time = time.time()
            test_proc = subprocess.Popen(cpu_pin_cmd, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            timer = Timer(args.timeout, test_proc.kill)
            try:
                timer.start()
                outs, errs = test_proc.communicate()
            finally:
                end_time = time.time()
                timer.cancel()
                if test_proc.returncode != 0:
                    file_run_times[test_file].append(-1)
                else:
                    file_run_times[test_file].append(end_time - start_time)
            #end_time = time.time()
            #file_run_times[test_file].append(end_time - start_time)
        with open(fig_name_out + ".data", 'w') as data_fd:
            data_fd.write("\n".join(map(str, file_run_times[test_file])))
        with open(fig_name_out + ".log", 'w') as log_fd:
            log_fd.write(f"Test {fig_name_out} iteration {str(i + 1)} of {repeat_count}.")
            log_fd.write(f"STDOUT:\n{outs}")
            log_fd.write(f"STDERR:\n{errs}")
    print()

# filter timeouts from plotting data
#pdb.set_trace()
file_run_times = {k:[x for x in v if x != -1] for k,v in file_run_times.items()}
file_warmup_times = {k:[x for x in v if x != -1] for k,v in file_warmup_times.items()}

# file_run_times = map(lambda x : filter(lambda y : y != -1, x), file_run_times)
# file_warmup_times = map(lambda x : filter(lambda y : y != -1, x), file_warmup_times)

for test_name,runtime_data in file_run_times.items():
    if not runtime_data:
        continue
    fig_name_out = out_folder + "/" + test_name + ".graph"
    fig_name_out = fig_name_out.replace("..", ".")
    debug_log(f"OUTPUT FOLDER: {fig_name_out}")
    # pdb.set_trace()
    if (args.plots):
        debug_log("Start histogram...")
        pfgraph.plot_histogram(runtime_data, bin_count, fig_name_out)
        debug_log("Start data point plot...")
        pfgraph.plot_data_points(runtime_data, fig_name_out)
        debug_log("Start sorted data point plot...")
        pfgraph.plot_sorted(runtime_data, fig_name_out)
    #emit_handler(pfstats.calc_median(runtime_data), stats_name_out)
    #emit_handler(pfstats.calc_mean(runtime_data), stats_name_out)
    if args.terminal:
        stats_fd = sys.stdout
    else:
        stats_name_out = f"{fig_name_out}.stats"
        stats_fd = open(stats_name_out, 'w')
    if args.input_file:
        file_name = args.input_file.name
    else:
        file_name = test_name
    stats_fd.write(f"File name: {file_name}\n");
    stats_fd.write(f"Data point count: {len(file_run_times['input'])}\n");
    debug_log("Start median...")
    pfstats.calc_median(runtime_data, stats_fd)
    debug_log("Start mean...")
    pfstats.calc_mean(runtime_data, stats_fd)
    debug_log("Start limits...")
    pfstats.calc_limits(runtime_data, stats_fd)
    if not args.terminal:
        stats_fd.close()
