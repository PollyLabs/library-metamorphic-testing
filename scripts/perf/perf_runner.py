#!/usr/bin/env python3
import datetime
import subprocess
import os

import psutil

timeout = 60
# test_path = "./out/test"
test_path = "./out/sleep.sh"
stat_log = "./out/test_stat.log"
# test_proc_cmd = ["timeout", timeout, test_path]
test_proc_cmd = ["sleep", "10"]

with open(stat_log, 'w') as stat_log_fd:
    # test_proc = subprocess.Popen(test_proc_cmd, stdout=subprocess.PIPE, encoding="utf-8")
    pid_stat_proc_cmd = ["pidstat", "-d", "-u", "-h", str(1), "-e", test_path]
    pid_stat_proc = subprocess.Popen(pid_stat_proc_cmd, stdout=stat_log_fd, stderr=stat_log_fd, encoding="utf-8")
    # test_proc.communicate()
    pid_stat_proc.communicate()

# test_proc_psutil = psutil.Process(test_proc_pid)
# test_proc_psutil_stats = \
    # ["cpu_time", "memory_full_info", "io_counters"]
# stats = test_proc_psutil.as_dict()
# print(stats)

print("done")
