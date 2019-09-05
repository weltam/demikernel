import pandas as pd

time_col = 'time ms'
lat_col = 'latency us'
write_col = 'write'
bs_col = 'bs'

avg_lat = 'average latency (us)'
avg_bw = 'average bandwidth (MB/s)'

bytes_per_mb = 1024 ** 2
total_time = 120

def load_files(files):
    res = []
    for finfo in files:
        tmp = {
            'dev': finfo['dev'],
            'qs': finfo['qs'],
            'bs': finfo['bs'],
            'df': pd.read_csv(finfo['path'], sep=',',
                names=[time_col, lat_col, write_col, bs_col]),
        }
        res.append(tmp)
    return res

def avg_lat_and_bw(dfs):
    for obj in dfs:
        dfs[avg_lat] = (obj['df'][lat_col] / 1000).mean()
        dfs[avg_bw] = (float(obj['df'][bs_col].sum()) / bytes_per_mb /
                total_time)
    return dfs

def avgs_to_df(dfs):
    datadict = {
        'queue depth': [],
        avg_lat: [],
        avg_bw: [],
        'device': []
    }
    for obj in dfs:
        datadict['device'] = obj['dev']
        datadict['queue depth'] = obj['qs']
        datadict['bs'] = obj['bs']
        datadict[avg_lat] = obj[avg_lat]
        datadict[avg_bw] = obj[avg_bw]

    return pd.DataFrame(datadict)

def dump_dfs_to_csv(dfs):
    for path, df in dfs:
        df.to_csv(path, index=False, sep=',')

def load_dfs_from_csv(paths):
    res = []
    for path in paths:
        res.append(pd.read_csv(path, sep=','))
    return res

def agg_and_save_dfs(in_files):
    dump_dfs_to_csv(
            {out_file: avgs_to_df(avg_lat_and_bw(load_files(in_files)))}
    )
