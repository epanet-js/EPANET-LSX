.PHONY: all init build test clean

CMAKE_DIR := build/cmake

all: build

# Fetch the EPANET engine sources into build/EPANET and apply every patch in
# patches/, in order. Patch application is idempotent: an already-applied patch
# is skipped, so re-running init on a prepared tree is safe.
init:
	./scripts/init.sh
	./scripts/apply-patches.sh

# Configure and build the epanet2 shared library into build/.
build: init
	cmake -S . -B $(CMAKE_DIR)
	cmake --build $(CMAKE_DIR)

# Build every *.test.cpp into a test_ binary, then run the suite.
test: build
	cmake --build $(CMAKE_DIR) -j8 --target build_tests
	./scripts/run-tests.sh

# Remove the build folder (EPANET sources, patch state, and build output).
clean:
	rm -rf build
