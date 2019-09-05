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

SSD_PCIE="0000.af.00.0"
SSD4096_PCIE="0000.b0.00.0"
OPT_PCIE="0000.12.00.0"

LIBAIO_MNT="/mnt/libaio_nvme"
LIBAIO_DEV="/dev/nvme3n1p1"
SYNC_MNT="/mnt/sync_nvme"
SYNC_DEV="/dev/nvme4n1p1"
SYNC_RAM_MNT="/mnt/sync_ramdisk"

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

        COMMON_OPTS="-f ${OP} -n ${NUM_REQS} -q ${QS} -t ${RWT}"
        COMMON_OPTS="${COMMON_OPTS} -i ${BS} -m ${MAX_FILE_SIZE}"
        # Add what we want to be output.
        COMMON_OPTS="${COMMON_OPTS} --latency-results"
        #COMMON_OPTS="${COMMON_OPTS} --poll-results"
        COMMON_OPTS="${COMMON_OPTS} --submission-results"
        #COMMON_OPTS="${COMMON_OPTS} --buffer-results"

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
        DRIVER_OPTS=""
        if [ ${DEV} == "spdk" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${SSD_PCIE} -n 1"
          COMMON_OPTS="${COMMON_OPTS} -c 10,30 -d spdk -a -M${SSD_PCIE}"
        elif [ ${DEV} == "spdk_delay" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${SSD_PCIE} -n 1 -d"
          COMMON_OPTS="${COMMON_OPTS} -c 10,30 -d spdk -a -M${SSD_PCIE}"
        elif [ ${DEV} == "spdk_batch" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${SSD_PCIE} -n 1"
          COMMON_OPTS="${COMMON_OPTS} -c 10,30 -d spdk -a -M${SSD_PCIE}"
        elif [ ${DEV} == "spdk_delay_batch" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${SSD_PCIE} -n 1 -d"
          COMMON_OPTS="${COMMON_OPTS} -c 10,30 -d spdk -a -M${SSD_PCIE}"
        elif [ ${DEV} == "spdk4096" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${SSD4096_PCIE} -n 1"
          COMMON_OPTS="${COMMON_OPTS} -c 10,30 -d spdk -a -M${SSD4096_PCIE}"
        elif [ ${DEV} == "spdk4096_delay" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${SSD4096_PCIE} -n 1 -d"
          COMMON_OPTS="${COMMON_OPTS} -c 10,30 -d spdk -a -M${SSD4096_PCIE}"
        elif [ ${DEV} == "spdk4096_batch" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${SSD4096_PCIE} -n 1"
          COMMON_OPTS="${COMMON_OPTS} -c 10,30 -d spdk -a -M${SSD4096_PCIE}"
        elif [ ${DEV} == "spdk4096_delay_batch" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${SSD4096_PCIE} -n 1 -d"
          COMMON_OPTS="${COMMON_OPTS} -c 10,30 -d spdk -a -M${SSD4096_PCIE}"
        elif [ ${DEV} == "spdk_optane" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${OPT_PCIE} -n 1"
          COMMON_OPTS="${COMMON_OPTS} -c 0,20 -d spdk -a -M${OPT_PCIE}"
        elif [ ${DEV} == "spdk_optane_delay" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${OPT_PCIE} -n 1 -d"
          COMMON_OPTS="${COMMON_OPTS} -c 0,20 -d spdk -a -M${OPT_PCIE}"
        elif [ ${DEV} == "spdk_optane_batch" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${OPT_PCIE} -n 1"
          COMMON_OPTS="${COMMON_OPTS} -c 0,20 -d spdk -a -b ${QS} -M${OPT_PCIE}"
        elif [ ${DEV} == "spdk_optane_delay_batch" ]; then
          DRIVER_OPTS="${DRIVER_OPTS} -t PCIe -a ${OPT_PCIE} -n 1 -d"
          COMMON_OPTS="${COMMON_OPTS} -c 0,20 -d spdk -a -b ${QS} -M${OPT_PCIE}"
        elif [ ${DEV} == "sync" ]; then
          COMMON_OPTS="${COMMON_OPTS} -d sync -p ${SYNC_MNT}/benchmark-test"
        elif [ ${DEV} == "sync_force_sync" ]; then
          COMMON_OPTS="${COMMON_OPTS} -d sync -p ${SYNC_MNT}/benchmark-test -s"
        elif [ ${DEV} == "sync_ramdisk" ]; then
          COMMON_OPTS="${COMMON_OPTS} -d sync -p ${SYNC_RAM_MNT}/benchmark-test -s"
        fi
        if [ -z "${DRIVER_OPTS}" ]; then
          echo ${BENCHMARK_PATH} ${COMMON_OPTS} > ${OP}/benchmark.out
          ${BENCHMARK_PATH} ${COMMON_OPTS} 2>&1 >> ${OP}/benchmark.out
        else
          echo "${BENCHMARK_PATH} ${COMMON_OPTS} -o \"${DRIVER_OPTS}\"" > ${OP}/benchmark.out
          ${BENCHMARK_PATH} ${COMMON_OPTS} -o "${DRIVER_OPTS}" 2>&1 >> ${OP}/benchmark.out
        fi
      done
    done
  done
done
