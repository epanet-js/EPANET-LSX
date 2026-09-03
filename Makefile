.PHONY: all init patch build clean

EPANET_DIR := build/EPANET
CMAKE_DIR  := build/cmake
PATCH      := $(CURDIR)/patches/0001-drive-lsx.patch

all: build

# Fetch the EPANET engine sources into build/EPANET.
init:
	./scripts/init.sh

# Apply the LSX integration patch to the EPANET sources (idempotent).
patch: init
	@if [ ! -d "$(EPANET_DIR)" ]; then \
		echo "EPANET sources missing at $(EPANET_DIR); run 'make init'"; exit 1; \
	fi
	@cd $(EPANET_DIR) && \
	if git apply --reverse --check "$(PATCH)" >/dev/null 2>&1; then \
		echo "LSX patch already applied."; \
	elif git apply --check "$(PATCH)" >/dev/null 2>&1; then \
		git apply "$(PATCH)" && echo "LSX patch applied."; \
	else \
		echo "ERROR: LSX patch does not apply cleanly to $(EPANET_DIR)."; exit 1; \
	fi

# Patch, then configure and build the epanet2 shared library into build/.
build: patch
	cmake -S . -B $(CMAKE_DIR)
	cmake --build $(CMAKE_DIR)

# Remove the build folder (EPANET sources, patch state, and build output).
clean:
	rm -rf build
