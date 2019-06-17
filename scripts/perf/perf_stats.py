import numpy as np

def calc_median(data, out_file):
    assert(out_file.writable())
    out_file.write(f"MEDIAN: {str(np.median(data))}\n")

def calc_mean(data, out_file):
    assert(out_file.writable())
    out_file.write(f"MEAN: {str(np.mean(data))}\n")
