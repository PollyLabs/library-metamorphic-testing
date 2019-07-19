import pdb
import numpy as np
import scipy.stats as sp_stats
import scipy.special as sp_special

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
    #pdb.set_trace()
    for sample in datas:
        ranked_samples.append(ranked_data[curr_index:curr_index+len(sample)])
        curr_index += len(sample)
    ranked_data_sum = [sum(x) for x in ranked_samples]
    return ranked_data_sum

def utest(data1, data2):
    return sp_stats.mannwhitneyu(data1, data2, alternative='two-sided')

def htest(*args):
    return sp_stats.kruskal(*args, nan_policy='raise')

def kl_div(data1, data2):
    return sp_special.kl_div(data1, data2)

# From https://gist.github.com/swayson/86c296aa354a555536e6765bbe726ff7
def kl_div_variant(data1, data2):
    p = np.asarray(data1, dtype = np.float)
    q = np.asarray(data2, dtype = np.float)
    return np.sum(np.where(p != 0, p * np.log(p / q), 0))
