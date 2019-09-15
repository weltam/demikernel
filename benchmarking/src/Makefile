CURDIR := $(shell pwd)
BUILD_DIR := $(abspath $(CURDIR)/../build)

PCM_DIR := $(abspath $(CURDIR)/../submodules/pcm)
PCM_INCLUDES := $(PCM_DIR)
PCM_OBJS_TMP := msr cpucounters pci mmio client_bw utils
PCM_OBJS := $(addprefix $(PCM_DIR)/,$(addsuffix .o,$(PCM_OBJS_TMP)))

SPDK_DIR := $(abspath $(CURDIR)/../submodules/spdk)
SPDK_INCLUDES := $(SPDK_DIR)/include
SPDK_LIBS_DIR := $(SPDK_DIR)/build/lib
SPDK_LIBS := \
		spdk_log spdk_nvme spdk_sock spdk_sock_posix spdk_thread spdk_vmd spdk_util
SPDK_LIBS2 := $(addprefix $(SPDK_LIBS_DIR)/,$(addsuffix .a,libspdk_env_dpdk))

DPDK_LIBS_DIR := $(SPDK_DIR)/dpdk/build/lib
DPDK_LIBS_TMP := \
		librte_eal librte_mempool librte_ring librte_mbuf librte_mempool_ring \
		librte_pci librte_bus_pci librte_kvargs librte_vhost librte_net \
		librte_hash librte_cryptodev
DPDK_LIBS := $(addprefix $(DPDK_LIBS_DIR)/,$(addsuffix .a,$(DPDK_LIBS_TMP)))

SYS_LIBS := -lnuma -ldl -luuid

CXXFLAGS += \
		-g -O2 -std=c++11 -Wall -Wextra -Wno-unused-parameter \
		-Wno-missing-field-initializers -Wmissing-declarations \
		-fno-strict-aliasing -fPIC -fstack-protector -fno-common -Wformat \
		-Wformat-security
LINKERFLAGS += -Wl,-z,relro,-z,now -Wl,-z,noexecstack

OBJS := \
		$(BUILD_DIR)/benchmark.o \
		$(BUILD_DIR)/benchmarkrunner.o \
		$(BUILD_DIR)/options.o \
		$(BUILD_DIR)/iodrivers/iodriver.o \
		$(BUILD_DIR)/iodrivers/spdk_nvme.o \
		$(BUILD_DIR)/iodrivers/sync_read_write.o \
		$(BUILD_DIR)/utils/fixed_queue.o \
		$(BUILD_DIR)/utils/parsing.o \
		$(BUILD_DIR)/utils/threading.o \
		$(BUILD_DIR)/utils/time_tracking.o \
		$(BUILD_DIR)/workgenerators/workgenerator.o

ifdef USE_PCM
OBJS += $(BUILD_DIR)/pcm/pcie_monitor.o
DEFS := -DUSE_PCM=1
PCM_LIBS := $(PCM_OBJS)
endif

ifdef BUFFER_LATENCY
DEFS += -DBUFFER_LATENCY=1
endif

ifdef SUBMIT_LATENCY
DEFS += -DSUBMIT_LATENCY=1
endif

ifdef POLL_LATENCY
DEFS += -DPOLL_LATENCY=1
endif

SRCS := $(OBJS:.o=.cpp)

# Use sort to get unique directories.
BUILD_DIRS := $(sort $(dir $(OBJS)))

.PHONY: clean
.DEFAULT_GOAL: $(BUILD_DIR)/benchmark

$(BUILD_DIR)/benchmark: \
		$(OBJS) \
		| $(BUILD_DIR)
	$(CXX) -o $@ $^ $(PCM_OBJS) $(LINKERFLAGS) -pthread \
		-L$(SPDK_LIBS_DIR) -Wl,--whole-archive $(addprefix -l,$(SPDK_LIBS)) \
		-Wl,--no-whole-archive $(SPDK_LIBS2) \
		-Wl,--whole-archive $(DPDK_LIBS) \
		-Wl,--no-whole-archive $(SYS_LIBS)

# Use second expansion to pull the directory out as a prereq. Just trying to use
# the pattern stem will always give the base build directory because the
# function is evaluated before the stem is determined.
.SECONDEXPANSION:
$(BUILD_DIR)/%.o: \
		%.cpp \
		| $$(@D)/
	$(CXX) $(CXXFLAGS) -I$(CURDIR) $(PCM_INC) $(SPDK_INC) $(DEFS) -c -o $@ $<

# Set extra includes here, but we can use the same pattern rule for this and
# regular iodrivers without the extra include.
$(BUILD_DIR)/iodrivers/spdk_nvme.o $(BUILD_DIR)/benchmarkrunner.o: \
		SPDK_INC := -I$(SPDK_INCLUDES)

$(BUILD_DIR)/pcm/%.o $(BUILD_DIR)/benchmark.o: PCM_INC := -I$(PCM_INCLUDES)

$(BUILD_DIRS):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
