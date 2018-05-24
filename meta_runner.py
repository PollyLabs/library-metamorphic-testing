#!/usr/bin/python
import subprocess

isl_tester_path = "./bin/isl_tester"
test_compile_dir = "./out"
test_compile_path = "./compile.sh"
test_run_path = "./out/test"
timeout = 30


gen_fail = []
test_fail = []
for seed in range(0, 10):
    seed = str(seed)
    timeout = str(timeout)
    generator_cmd = ["timeout", timeout, isl_tester_path, "-m", "SET_META", "-s", seed]
    generator_proc = subprocess.Popen(generator_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    out, err = generator_proc.communicate()
    if generator_proc.returncode != 0:
        gen_fail.append(seed)
    print("== Run seed " + seed)
    # print("GENOUT--\n" + out)
    # print("GENERR--\n" + err)

    try:
        compile_cmd = [test_compile_path]
        compile_proc = subprocess.run(compile_cmd, shell=True, check=True, cwd=test_compile_dir)
    except subprocess.CalledProcessError:
        print("Failed compiling.")
        continue

    test_cmd = ["timeout", timeout, test_run_path]
    test_proc = subprocess.Popen(test_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    out, err = test_proc.communicate()
    if test_proc.returncode != 0:
        test_fail.append(seed)
    # print("TESTOUT--\n" + out)
    # print("TESTERR--\n" + err)

print("GENFAIL: " + ",".join(gen_fail))
print("TESTFAIL: " + ",".join(test_fail))
