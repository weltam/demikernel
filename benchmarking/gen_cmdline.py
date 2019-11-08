import sys

samsung_addr = '0000.02.00.0'
optane_addr = '0000.17.00.0'
benchmark_cores = '0,10'

target_opts = {
  'spdk': {
    'io-driver': 'spdk',
    'core-mask': benchmark_cores,
    'async-completions': '',
    'spdk_opts': {
      'transport-type': 'PCIe',
      'device-address': samsung_addr,
      'namespace': '1',
    },
  },
  'spdk_delay': {
    'io-driver': 'spdk',
    'core-mask': benchmark_cores,
    'async-completions': '',
    'spdk_opts': {
      'transport-type': 'PCIe',
      'device-address': samsung_addr,
      'namespace': '1',
      'delay-doorbell': '',
    },
  },
  'spdk_delay_batch': {
    'io-driver': 'spdk',
    'core-mask': benchmark_cores,
    'async-completions': '',
    'spdk_opts': {
      'transport-type': 'PCIe',
      'device-address': samsung_addr,
      'namespace': '1',
      'delay-doorbell': '',
    },
    'complete-batch': '',
  },
  'spdk_batch': {
    'io-driver': 'spdk',
    'core-mask': benchmark_cores,
    'async-completions': '',
    'spdk_opts': {
      'transport-type': 'PCIe',
      'device-address': samsung_addr,
      'namespace': '1',
    },
    'complete-batch': '',
  },
  # Only 1 SSD available on UW workstation.
  #'spdk4096': {
  #  'io-driver': 'spdk',
  #  'transport-type': 'PCIe',
  #  'device-address': '0000.b0.00.0',
  #},
  #'spdk4096_delay': {
  #  'io-driver': 'spdk',
  #  'transport-type': 'PCIe',
  #  'device-address': '0000.b0.00.0',
  #},
  #'spdk4096_delay_batch': {
  #  'io-driver': 'spdk',
  #  'transport-type': 'PCIe',
  #  'device-address': '0000.b0.00.0',
  #},
  'spdk4096_batch': {
    'io-driver': 'spdk',
    'core-mask': benchmark_cores,
    'async-completions': '',
    'spdk_opts': {
      'transport-type': 'PCIe',
      'device-address': samsung_addr,
      'namespace': '1',
    },
    'complete-batch': '',
  },
  'spdk_optane': {
    'io-driver': 'spdk',
    'core-mask': benchmark_cores,
    'async-completions': '',
    'spdk_opts': {
      'transport-type': 'PCIe',
      'device-address': optane_addr,
      'namespace': '1',
    },
  },
  'spdk_optane_delay': {
    'io-driver': 'spdk',
    'core-mask': benchmark_cores,
    'async-completions': '',
    'spdk_opts': {
      'transport-type': 'PCIe',
      'device-address': optane_addr,
      'namespace': '1',
      'delay-doorbell': '',
    },
  },
  'spdk_optane_delay_batch': {
    'io-driver': 'spdk',
    'core-mask': benchmark_cores,
    'async-completions': '',
    'spdk_opts': {
      'transport-type': 'PCIe',
      'device-address': optane_addr,
      'namespace': '1',
      'delay-doorbell': '',
    },
    'complete-batch': '',
  },
  'spdk_optane_batch': {
    'io-driver': 'spdk',
    'core-mask': benchmark_cores,
    'async-completions': '',
    'spdk_opts': {
      'transport-type': 'PCIe',
      'device-address': optane_addr,
      'namespace': '1',
    },
    'complete-batch': '',
  },
  #'sync': {
  #  'io-driver': 'sync',
  #  'core-mask': benchmark_cores,
  #},
  #'sync_force_sync': {
  #  'io-driver': 'sync',
  #  'core-mask': benchmark_cores,
  #},
  #'sync_optane_force_sync': {
  #  'io-driver': 'sync',
  #  'core-mask': benchmark_cores,
  #},
  #'spdk_bdev': {
  #  'io-driver': 'sync',
  #  'core-mask': benchmark_cores,
  #},
  #'libaio': {
  #  'io-driver': 'sync',
  #  'core-mask': benchmark_cores,
  #},
  #'sync_nvme': {
  #  'io-driver': 'sync',
  #  'core-mask': benchmark_cores,
  #},
  #'sync_nd_nvme': {
  #  'io-driver': 'sync',
  #  'core-mask': benchmark_cores,
  #},
  #'sync_ramdisk': {
  #  'io-driver': 'sync',
  #  'core-mask': benchmark_cores,
  #},
}

#LIBAIO_MNT='/mnt/libaio_nvme'
#LIBAIO_DEV='/io-driver/nvme3n1p1'
#SYNC_MNT='/mnt/sync_nvme'
#SYNC_DEV='/io-driver/nvme4n1p1'
#SYNC_RAM_MNT='/mnt/sync_ramdisk'

# Args (in order) are:
#   1.  output path
#   2.  io latency (total)
#   3.  poll latency
#   4.  submit latency
#   5.  buffer latency
#   6.  PCI profiling
#   7.  number requests
#   8.  target
#   9.  operation type/pattern
#   10  max queue depth
#   11. block size
#   12. max file size
#   13. force sync
#   14. io-bytes to limit at
#   15. true to limit on numRequests
if len(sys.argv) < 15:
  print("wrong number of arguments\n")
  sys.exit(-1)

common_opts = ''
spdk_opts = ''

# Build generic command line.
common_opts += '--output-file-prefix ' + sys.argv[1] + ' '
common_opts += '--io-type ' + sys.argv[9] + ' '
common_opts += '--queue-depth ' + sys.argv[10] + ' '
common_opts += '--io-size ' + sys.argv[11] + ' '
common_opts += '--max-file-size ' + sys.argv[12] + ' '
if sys.argv[13] == 'true':
  common_opts += '--force-sync '

if sys.argv[2] == 'true':
  common_opts += '--latency-results '
if sys.argv[3] == 'true':
  common_opts += '--poll-results '
if sys.argv[4] == 'true':
  common_opts += '--submission-results '
if sys.argv[5] == 'true':
  common_opts += '--buffer-results '

if sys.argv[15] == 'true':
    common_opts += '--num-requests ' + sys.argv[7] + ' '
else:
    common_opts += '--io-bytes ' + sys.argv[14] + ' '

# Build spdk specific command line.
if target_opts[sys.argv[8]]['io-driver'] == 'spdk':
  if sys.argv[6] == 'true':
    common_opts += ('--monitor-device ' +
                    target_opts[sys.argv[8]]['spdk_opts']['device-address'] +
                    ' ')
    target_opts[sys.argv[8]]['core-mask'] = ('1,' +
        target_opts[sys.argv[8]]['core-mask'])

  for key, value in target_opts[sys.argv[8]].iteritems():
    if key == 'spdk_opts':
      for subkey, subvalue in value.iteritems():
        spdk_opts += '--' + subkey + ' ' + subvalue + ' '
    elif key == 'complete-batch':
      common_opts += '--' + key + ' ' + sys.argv[10] + ' '
    else:
      common_opts += '--' + key + ' ' + value + ' '

  common_opts += '--io-driver-opts ' + '"' + spdk_opts.strip() + '"'

print(common_opts.strip())
sys.exit(0)
