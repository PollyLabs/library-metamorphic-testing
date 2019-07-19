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
parser.add_argument("--cpu", type=int, default=1,
    help = "Which cpu core to pin the process to; core id passed to `taskset`.")
parser.add_argument("--timeout", type=int, default=60,
    help = "Timeout for each individual test execution.")
parser.add_argument("--repeat_count", type=int, default=200,
    help = "Number of times to execute one test.")
parser.add_argument("--warmup_ratio", type=float, default=10.0,
    help = "Percent of total number of iterations to perform as warmup.")
parser.add_argument("--bin_ratio", type=float, default=5.0,
    help = "Number of total bins for histogram, as percent of total tests.")
parser.add_argument("--input_file", type=argparse.FileType('r'), default=None,
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

bin_count = int(args.repeat_count * args.bin_ratio / 100)

#test_dir = "./perf/"
test_dir = "./tests/"
#file_run = [x for x in glob.glob(test_dir + "*") if os.path.splitext(x)[1] == "" and "graph" not in x]
file_run = [x for x in glob.glob(test_dir + "**/*.smt2") if os.path.splitext(x)[1] == ".smt2" and not "rewrite" in x]
file_run = [x for x in file_run if "1192684" in x]
#for x in file_run:
#    print(x)
#sys.exit(1)
file_run_times = {}
file_warmup_times = {}

out_folder = "./perf/graphs"
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
        try:
            os.makedirs(os.path.split(fig_name_out)[0])
        except FileExistsError:
            pass
        cpu_pin_cmd = ["taskset", str(args.cpu), "./z3", test_file]
        # Run training for hardware
        for i in range(0, int(args.repeat_count * args.warmup_ratio / 100)):
            print("Training " + test_file + " iteration " + str(i), end="\r")
            start_time = time.time()
            test_proc = subprocess.Popen(cpu_pin_cmd)
            timer = Timer(args.timeout, test_proc.kill)
            try:
                timer.start()
                test_proc.communicate()
                file_warmup_times[test_file].append(-1)
            finally:
                end_time = time.time()
                file_warmup_times[test_file].append(end_time - start_time)
                timer.cancel()
        print()
        with open(fig_name_out + ".warmup", 'w') as warmup_fd:
            warmup_fd.write("\n".join(map(str, file_warmup_times[test_file])))
        for i in range(0, args.repeat_count):
            print("Running " + test_file + " iteration " + str(i), end="\r")
            start_time = time.time()
            test_proc = subprocess.Popen(cpu_pin_cmd)
            timer = Timer(args.timeout, test_proc.kill)
            try:
                timer.start()
                test_proc.communicate()
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
    stats_name_out = f"{fig_name_out}.stats"
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
    with open(stats_name_out, 'w') as stats_fd:
        debug_log("Start median...")
        pfstats.calc_median(runtime_data, stats_fd)
        debug_log("Start mean...")
        pfstats.calc_mean(runtime_data, stats_fd)
