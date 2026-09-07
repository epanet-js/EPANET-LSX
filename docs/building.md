# Building EPANET-LSX

## Required dependencies

- A C and C++ toolchain (GCC, Clang, or MSVC); the tests require C++20.
- [CMake](https://cmake.org/) 3.16 or newer.
- `git` and `curl` (used to fetch the pinned EPANET sources and the test runner).
- `sha256sum` or `shasum` (to verify downloads).
- GNU `make` to drive the workflow below.

The EPANET engine sources are fetched automatically (pinned to `v2.3.5`) and
patched during `make init`.

## How to build

```bash
make
```

This fetches and patches the EPANET sources, then configures and builds with
CMake. The outputs land in `build/`:

- `build/libepanet2.so`: the EPANET engine shared library with Lua scripting,
  a drop-in replacement for the stock EPANET library.
- `build/runepanet`: the command-line runner.

Run a model from the command line:

```bash
./build/runepanet model.inp report.rpt [output.out]
```

To start from a clean tree:

```bash
make clean   # remove build/ (fetched sources, patch state, and build output)
make
```

## How to run tests

```bash
make test
```

This builds every `tests/**/*.test.cpp` into its own binary and runs the suite.
Tests are built with AddressSanitizer/LeakSanitizer so memory errors and leaks
surface as failures. There are two layers:

- **Unit tests** (`tests/unit`): per-module coverage of invidual modules.
- **Acceptance tests** (`tests/acceptance`): real `.inp` models driven through
  the engine, asserting the scripted control behaviour.
