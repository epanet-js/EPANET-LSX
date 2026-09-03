.PHONY: all init patch build clean

CMAKE_DIR := build/cmake

all: build

# Fetch the EPANET engine sources into build/EPANET.
init:
	./scripts/init.sh

# Apply every patch in patches/ to the EPANET sources, in order (idempotent).
patch: init
	./scripts/apply-patches.sh

# Patch, then configure and build the epanet2 shared library into build/.
build: patch
	cmake -S . -B $(CMAKE_DIR)
	cmake --build $(CMAKE_DIR)

# Remove the build folder (EPANET sources, patch state, and build output).
clean:
	rm -rf build
