import pandas as pd

time_col = 'time ms'
lat_col = 'latency us'
write_col = 'write'
bs_col = 'bs'

def read_files(files):
  dfs = []

  for f in files:
    df = pd.read_csv(f, sep=',', names=[time_col, lat_col, write_col, bs_col])
    df.drop(columns=[write_col], inplace=True)
    df[lat_col] = df[lat_col] / 1000
    dfs.append(df)
  return dfs

def max_by_bs(dfs):
  max_by_bs = {}
  for df in dfs:
    max_by_bs[df.at[0, bs_col]] = df[lat_col].max()
  return max_by_bs

def min_by_bs(dfs):
  min_by_bs = {}
  for df in dfs:
    min_by_bs[df.at[0, bs_col]] = df[lat_col].min()
  return min_by_bs


def stddev_by_bs(dfs):
  stddev_by_bs = {}
  for df in dfs:
    stddev_by_bs[df.at[0, bs_col]] = df[lat_col].std()
  return stddev_by_bs

def trim_to_percentile(dfs, p=0.95):
  b = []
  for df in dfs:
    df = df[df[lat_col] < df[lat_col].quantile(p)]
    b.append(df)
  return b

def downsample(dfs, num_samples=750, rand=42):
  ds = []
  for df in dfs:
    ds.append(df.sample(n=num_samples, random_state=rand))
  return ds

def merge_blk_sizes(dfs):
  ret = pd.DataFrame()

  for df in dfs:
    ret = ret.append(df)
  return ret

def plot_quick_prepare(files, p=0.95):
  dfs = read_files(files)
  percentiles = trim_to_percentile(dfs, p)
  ds = downsample(percentiles)
  return merge_blk_sizes(ds)

def lineplot_all(files):
  dfs = read_files(files)

  for df in dfs:
    avged = df[lat_col].groupby(df[time_col]).mean().to_frame().reset_index()
