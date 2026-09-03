.PHONY: all init patch build clean

EPANET_DIR := build/EPANET
CMAKE_DIR  := build/cmake
PATCH_DIR  := $(CURDIR)/patches

all: build

# Fetch the EPANET engine sources into build/EPANET.
init:
	./scripts/init.sh

# Apply every patch in patches/ to the EPANET sources, in order (idempotent).
patch: init
	@if [ ! -d "$(EPANET_DIR)" ]; then \
		echo "EPANET sources missing at $(EPANET_DIR); run 'make init'"; exit 1; \
	fi
	@cd $(EPANET_DIR) && \
	for p in $$(ls "$(PATCH_DIR)"/*.patch 2>/dev/null | sort); do \
		name=$$(basename "$$p"); \
		if git apply --reverse --check "$$p" >/dev/null 2>&1; then \
			echo "$$name already applied."; \
		elif git apply --check "$$p" >/dev/null 2>&1; then \
			git apply "$$p" && echo "$$name applied."; \
		else \
			echo "ERROR: $$name does not apply cleanly to $(EPANET_DIR)."; exit 1; \
		fi; \
	done

# Patch, then configure and build the epanet2 shared library into build/.
build: patch
	cmake -S . -B $(CMAKE_DIR)
	cmake --build $(CMAKE_DIR)

# Remove the build folder (EPANET sources, patch state, and build output).
clean:
	rm -rf build
