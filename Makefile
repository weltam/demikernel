# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

export PREFIX ?= $(HOME)

# export PKG_CONFIG_PATH ?= $(shell find $(PREFIX)/lib/ -name '*pkgconfig*' -type d)
export LD_LIBRARY_PATH ?= $(shell find /usr/local/lib/ -name '*aarch64-linux-gnu*' -type d)
# export LD_LIBRARY_PATH ?= $(HOME)/aarch64-linux-gnu
export CONFIG_PATH ?= $(HOME)/config.yaml

export CARGO ?= $(HOME)/.cargo/bin/cargo
export TIMEOUT ?= 30

export SRCDIR = $(CURDIR)/src
export BINDIR = $(CURDIR)/bin
export LIBDIR = $(CURDIR)/lib
export CONTRIBDIR = $(CURDIR)/submodules
export BUILDDIR = $(CURDIR)/build

#===============================================================================

export DRIVER ?= $(shell [ ! -z "`lspci | grep -E "ConnectX-[4,5]"`" ] && echo mlx5 || echo mlx4)
export BUILD ?= --release

#===============================================================================

all: demikernel-all demikernel-tests

clean: demikernel-clean

demikernel-all: demikernel-catnip

demikernel-catnip:
	cd $(SRCDIR) && \
	$(CARGO) build $(BUILD) -p catnip-libos $(CARGO_FLAGS)
	# $(CARGO) build $(BUILD) --features=$(DRIVER) -p catnip-libos $(CARGO_FLAGS)

demikernel-catnap:
	cd $(SRCDIR) && \
	$(CARGO) build $(BUILD) -p catnap-libos $(CARGO_FLAGS)

demikernel-tests:
	cd $(SRCDIR) && \
	$(CARGO) build --tests $(BUILD) --features=$(DRIVER) $(CARGO_FLAGS)

demikernel-clean:
	cd $(SRCDIR) &&   \
	rm -rf target &&  \
	$(CARGO) clean && \
	rm -f Cargo.lock

test: test-catnip

test-catnip:
	cd $(SRCDIR) && \
	sudo -E LD_LIBRARY_PATH="$(LD_LIBRARY_PATH)" timeout $(TIMEOUT) $(CARGO) test $(BUILD) $(CARGO_FLAGS) -p catnip-libos -- --nocapture $(TEST)
	# sudo -E LD_LIBRARY_PATH="$(LD_LIBRARY_PATH)" timeout $(TIMEOUT) $(CARGO) test $(BUILD) --features=$(DRIVER) $(CARGO_FLAGS) -p catnip-libos -- --nocapture $(TEST)

test-catnap:
	cd $(SRCDIR) && \
	sudo -E LD_LIBRARY_PATH="$(LD_LIBRARY_PATH)" timeout $(TIMEOUT) $(CARGO) test $(BUILD) $(CARGO_FLAGS) -p catnap-libos -- --nocapture $(TEST)