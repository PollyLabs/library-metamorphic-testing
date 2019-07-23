import matplotlib.pyplot as plt

def plot_data_points(data, fig_name):
    plt.figure(num = 1, figsize = (30, 5))
    plt.suptitle(fig_name)
    plt.plot(data, "kx")
    plt.xlabel("Test run #")
    plt.ylabel("Test runtime")
    plt.grid(b=True, which="major", axis="y", alpha=0.1, color='k')
    plt.savefig(f"{fig_name}_plot.png")
    plt.clf()

def plot_histogram(data, bin_count, fig_name):
    plt.figure(num = 1, figsize = (10, 10))
    plt.suptitle(fig_name)
    plt.hist(data, bins=bin_count)
    plt.xlabel("Runtime")
    plt.ylabel("Total test count")
    plt.savefig(f"{fig_name}_hist.png")
    plt.clf()

def plot_sorted(data, fig_name):
    plot_data_points(sorted(data), f"{fig_name}_sorted")
