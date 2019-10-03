#! /bin/bash

OUTPUT_DIR="/h/ashlie/new-benchmark-testing"

BENCHMARK_PATH="/h/ashlie/benchmarking/build/benchmark"

RW=(
  "write"
  #"randwrite"
  #"read"
  #"randread"
)

DEVS=(
  #"spdk"
  #"spdk_delay"
  #"spdk_delay_batch"
  "spdk_batch"
  #"spdk4096"
  #"spdk4096_delay"
  #"spdk4096_delay_batch"
  #"spdk4096_batch"
  #"spdk_optane"
  #"spdk_optane_delay"
  #"spdk_optane_delay_batch"
  #"spdk_optane_batch"
  #"sync"
  #"sync_force_sync"
  #"sync_optane_force_sync"
  #"spdk_bdev"
  #"libaio"
  #"sync_nvme"
  #"sync_nd_nvme"
  #"sync_ramdisk"
)

MAX_FILE_SIZE=21474836480

BLK_SIZES=(
  512
  #1024
  #2048
  #4096
  #8192
)

QSIZE=(
  1
  #2
  #4
  8
  #16
  #32
  64
  #128
  #256
  512
  #1024
  #2048
  4096
)

NUM_REQS=1000000

RLAT=true
RPOLL=true
RSUB=true
RBUF=false
RPCI=false
FSYNC=false

set -e

for RWT in ${RW[@]}; do
  for DEV in ${DEVS[@]}; do
    for QS in ${QSIZE[@]}; do
      for BS in ${BLK_SIZES[@]}; do
        if [ ${DEV} == "spdk4096" -a ${BS} -ne 4096 ]; then
          continue
        elif [ ${DEV} == "spdk4096_batch" -a ${BS} -ne 4096 ]; then
          continue
        elif [ ${DEV} == "spdk4096_delay" -a ${BS} -ne 4096 ]; then
          continue
        elif [ ${DEV} == "spdk4096_delay_batch" -a ${BS} -ne 4096 ]; then
          continue
        fi
        OP="${OUTPUT_DIR}/${RWT}/q${QS}/${DEV}/bs${BS}/"

        mkdir -p "${OP}"

        # Unmount fs, reformat, and remount.
        if [ ${DEV} == "libaio" ]; then
          umount ${LIBAIO_MNT}
          yes | mkfs -t ext4 ${LIBAIO_DEV}
          mount -t ext4 ${LIBAIO_DEV} ${LIBAIO_MNT}
        elif [ ${DEV} == "sync" -o ${DEV} == "sync_force_sync" ]; then
          umount ${SYNC_MNT}
          yes | mkfs -t ext4 ${SYNC_DEV}
          mount -t ext4 ${SYNC_DEV} ${SYNC_MNT}
        elif [ ${DEV} == "sync_optane" -o ${DEV} == "sync_optane_force_sync" ]; then
          umount ${SYNC_MNT}
          yes | mkfs -t ext4 ${SYNC_DEV}
          mount -t ext4 ${SYNC_DEV} ${SYNC_MNT}
        elif [ ${DEV} == "sync_nd_nvme" ]; then
          umount ${SYNC_MNT}
          yes | mkfs -t ext4 ${SYNC_DEV}
          mount -t ext4 ${SYNC_DEV} ${SYNC_MNT}
        elif [ ${DEV} == "sync_ramdisk" ]; then
          umount ${SYNC_RAM_MNT}
          mount -t tmpfs -o size=24g tmpfs ${SYNC_RAM_MNT}
        fi

        # Actually run things.

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
        ALL_OPTS=$(python ./gen_cmdline.py ${OP} ${RLAT} ${RPOLL} ${RSUB} \
          ${RBUF}  ${RPCI} ${NUM_REQS} ${DEV} ${RWT} ${QS} ${BS} \
          ${MAX_FILE_SIZE} ${FSYNC})
        echo "${BENCHMARK_PATH} ${ALL_OPTS}" > ${OP}/benchmark.out
        eval "${BENCHMARK_PATH} ${ALL_OPTS} 2>&1 >> ${OP}/benchmark.out"
      done
    done
  done
done
