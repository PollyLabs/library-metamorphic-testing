import pdb
import numpy as np
import scipy.stats as sp_stats

def calc_median(data, out_file):
    assert(out_file.writable())
    out_file.write(f"MEDIAN: {str(np.median(data))}\n")

def calc_mean(data, out_file):
    assert(out_file.writable())
    out_file.write(f"MEAN: {str(np.mean(data))}\n")

def calc_samples_sum_rank(datas, out_file):
    datas = [[1, 5, 20], [0, 7, 10]]
    ranked_data = sp_stats.rankdata(datas)
    ranked_samples = []
    curr_index = 0
    pdb.set_trace()
    for sample in datas:
        ranked_samples.append(ranked_data[curr_index:curr_index+len(sample)])
        curr_index += len(sample)
    ranked_data_sum = [sum(x) for x in ranked_samples]
    return ranked_data_sum
