# Testing Guidelines

We use two testing layers:
- **Unit tests** per module.
- **Acceptance tests** that link the whole engine and check a real `.inp` (with an embedded Lua script) and verifies results.

## File naming and layout

- One unit-test file per module, named **`module-name.test.cpp`** (matching the
  module, for example: `lsx-script.test.cpp`, `lsx-sandbox.test.cpp`,
  `lsx-toolkit-binding.test.cpp`, …).
- Acceptance tests in their own `*.test.cpp` file(s), e.g.
  `acceptance.test.cpp`, using fixture `.inp` models.
- Example tree:
  ```
  tests/
  ├── unit/
  │   ├── lsx-script.test.cpp
  │   ├── lsx-sandbox.test.cpp
  │   ├── lsx-hooks.test.cpp
  │   └── lsx-toolkit-binding.test.cpp
  ├── acceptance/
  │   └── acceptance.test.cpp
  └── fixtures/            # sample .inp inputs + expected outputs
      ├── net-no-lua.inp
      ├── net-with-lua.inp
      └── expected/…
  ```

## Building the tests (CMake: one binary per test file)

Build and run all tests from the Makefile:

`make test`

## Unit tests

Test each module's public (opaque-struct) surface in isolation.

**Heap coverage.** Every module that allocates has a test exercising the
**create/destroy pair**, run under ASan so leaks surface. Destructors must accept
`NULL`.

**Thread-safety coverage.** Because the engine must run concurrent projects
([Development Guidelines](./development-guidelines.md)), include a test that runs
two independent project contexts and asserts they do not observe each other's
state. Running the suite under ASan/TSan makes accidental shared state show up.

## Stub injection for external calls

Make side-effecting calls (file reads/writes, clock, RNG) testable via
**function-pointer seams**. Because mutable static state is forbidden, the seam is
**stored in the project context**, not in a module-level `static`.

Production code should initialize it to the real libc implementation, tests initialize it to a
stub they observe. This keeps the seam reentrant across concurrent projects.

Example:
```c
// lsx_io.h
#pragma once
#include <stddef.h>
#include <stdint.h>

typedef int (*Lsx_WriteBytesFn)(const char *path, const uint8_t *data, size_t size);

// The seam lives on the context, set at construction; no module-level static.
void Lsx_Io_SetWriteBytesFn(LsxContext *ctx, Lsx_WriteBytesFn fn);
int  Lsx_Io_WriteBytes(LsxContext *ctx, const char *path,
                       const uint8_t *data, size_t size);
```

```cpp
static int lastSize = 0;
static const char *lastPath = nullptr;
static int stubWrite(const char *path, const uint8_t *data, size_t size) {
    lastPath = path; lastSize = (int)size; return 0;   // capture, no disk
}

describe("Lsx_Io_WriteBytes", []() {
    it("passes path and size through", [&]() {
        LsxContext *ctx = Lsx_Context_New();
        Lsx_Io_SetWriteBytesFn(ctx, stubWrite);
        uint8_t buf[4] = {1, 2, 3, 4};
        Lsx_Io_WriteBytes(ctx, "out.bin", buf, sizeof(buf));
        expect(lastSize).toBe(4);
        Lsx_Context_Free(ctx);
    });
});
```

Seam guidelines:
- Apply to boundaries that are awkward in a test: file reads, file writes, time,
  randomness. One field per swappable call; keep the interface tiny.
- The seam is for **boundaries**, not internal logic. Test pure functions (parse,
  argument marshalling) directly.
- Because the seam lives on the context, there is nothing global to reset between
  tests; each test constructs its own context.

## Sanitizers

- Test are built with **ASan/LSan** (and ideally UBSan).

## What good coverage looks like

- Every module: constructor/destructor, happy path, one malformed/edge case.
- Thread safety: two concurrent contexts stay independent.
- Acceptance: no-Lua parity with stock EPANET, plus at least one scripted model.
