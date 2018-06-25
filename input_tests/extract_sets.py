#!/usr/bin/python
import re

input_file = "./test_autotuner.INFO"
output_file = "./input_sets_autotuner"
regexp = re.compile("\[.*\] -> { (?!.*->).*:.* }")
found_sets = set()

with open(input_file, 'r') as file_reader:
    for line in file_reader:
        result = regexp.search(line)
        if result:
            found_sets.add(result.group(0))

with open(output_file, 'w') as file_writer:
    file_writer.writelines("\n".join(found_sets))
