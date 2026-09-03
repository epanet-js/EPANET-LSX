# Contributing to EPANET-LSX

Thanks for your interest in contributing to **EPANET-LSX** — the Lua Scripting
Extensions for [EPANET](https://github.com/OpenWaterAnalytics/EPANET).

EPANET-LSX lets users build a version of the EPANET engine shared libraries that
can execute an extra Lua script embedded inside the `.inp` project file. This
guide explains how to get set up, how we work, and what we expect from a
contribution before it is merged.

## Table of contents

- [Project goals](#project-goals)
- [Ways to contribute](#ways-to-contribute)
- [Development setup](#development-setup)
- [Building the shared libraries](#building-the-shared-libraries)
- [Coding guidelines](#coding-guidelines)
- [Testing](#testing)
- [Commit and pull request workflow](#commit-and-pull-request-workflow)
- [Reporting bugs and requesting features](#reporting-bugs-and-requesting-features)
- [License](#license)

## Project goals

EPANET-LSX aims to:

- Provide a sandboxed Lua scripting interface embedded in the `.inp`
  project file.
- Extend the EPANET engine without changing its behaviour. A model
  that does not use the Lua extension must behave exactly like stock EPANET.
- Ship as shared libraries (`.so` / `.dll` / `.dylib`) that can be switched
  in-place of the original EPANET libraries.
- Maximize portability to multiple platforms.

## Ways to contribute

- **Report bugs** and unexpected simulation results.
- **Propose features** for the Lua scripting surface.
- **Improve documentation** under [`/docs`](./docs).
- **Submit code** — bug fixes, new bindings, build/packaging improvements.
- **Add tests** that cover untested behaviour.

Small fixes (typos, docs, obvious bugs) can go straight to a pull request. For
anything larger — new Lua APIs, changes to the embedding format, build system
changes — please open an issue first so we can agree on the approach before you
invest time.

## Development setup

1. **Fork** the repository and clone your fork.
2. Create a topic branch off `main`:
   ```bash
   git checkout -b feature/short-description
   ```
3. Install the toolchain (see below).

### Prerequisites

- A C/C++ toolchain (GCC, Clang, or MSVC).
- CMake (build system).
- Git.

## Coding guidelines

- Keep changes to the EPANET code minimal to ensure compatibility across different
  EPANET versions.
- Changes to the EPANET code should be applied as incremental patches to the base code.
- Guard LSX behaviour in EPANET to ensure that non-scripted models are unaffected.
- Document public functions and any new Lua-facing API.
- Prefer small, focused commits and pull requests.

Before submitting code, read the
[Development Guidelines](./docs/development-guidelines.md) — in particular the
rule that **mutable static/global memory is forbidden** for thread safety. The
full coding style, library choices, and API conventions are maintained as
separate documents under [`/docs`](./docs).

## Testing

Every behavioural change should come with tests, and every bug fix should come
with a test that fails before the fix and passes after it.

Detailed testing guidelines — frameworks, how to run the suite, how to add
fixtures and reference models — are in the
[Testing Guidelines](./docs/testing-guidelines.md).

## Commit and pull request workflow

- Write clear, imperative commit messages ("Add Lua hook for node results", not
  "added stuff").
- Keep each pull request focused on a single concern.
- Ensure the project builds and the tests pass before opening the PR.
- Reference any related issue in the PR description.
- Be responsive to review feedback — we aim to review promptly in return.

By contributing, you agree that your contributions are licensed under the
project's [MIT License](./LICENSE).

## Reporting bugs and requesting features

Open an issue and include, where relevant:

- What you expected to happen and what actually happened.
- A minimal `.inp` file (with the embedded Lua script) that reproduces the
  problem.
- Your platform, compiler, and build configuration.
- Steps to reproduce.

## License

EPANET-LSX is released under the [MIT License](./LICENSE), following the same license as [EPANET](https://github.com/OpenWaterAnalytics/EPANET/blob/dev/LICENSE).
All contributions are accepted under the same license.
