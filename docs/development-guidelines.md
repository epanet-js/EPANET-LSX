# Development Guidelines

## Language & building

- Target the same C standard as upstream EPANET. Avoid using GNU/compiler extensions.
- CMake for builds.
- Prefer the C standard library and the EPANET toolkit API over
  platform-specific calls. Where a platform API is unavoidable, isolate it
  behind a small wrapper.
- Keep changes to the upstream EPANET code minimal and expressed as incremental
  patches, to make applying them to newer EPANET versions easier.

## Memory & thread safety

EPANET may run multiple independent projects concurrently, one per thread.
Thread safety is a hard requirement.

**Static and global mutable memory is forbidden.**

- Do not introduce `static` local variables that hold mutable state, mutable
  globals, or file-scope mutable buffers. They are shared across threads and
  make the library non-reentrant.
- All per-run state must live in a context/project struct that is passed
  explicitly through the call chain (the same way EPANET does with its project
  handle). The Lua interpreter state (`lua_State`) is part of that per-project
  context and must never be shared between projects or threads.
- `const` compile-time constants like lookup tables, string literals or enum values
  are fine.
- Anything that must persist for the lifetime of a run is heap-allocated and
  owned by the project context, created in its constructor and released in its
  destructor.

**Allocation discipline:**

- Every allocation needs a matching destructor path, freed in reverse
  acquisition order.
- Destructors must accept `NULL` harmlessly and must be covered by tests.
- Prefer stack allocation for short-lived, bounded, single-threaded-scope data.
  Use the heap (owned by the context) for anything that outlives a call or is
  unbounded in size.

## Error handling

- Use EPANET-style `ErrorCode` return values / out-parameters for recoverable
  failures rather than aborting.
- Report user-facing diagnostics through the engine's existing error/reporting
  channel so host applications receive them consistently.
- Stop processing on malformed input with an actionable message (what failed,
  and where in the script if known).

## Code style

Favor readability over clever tricks or premature optimization. Code is
read far more often than it is written.

**Naming conventions:**

- Public functions: `Module_Function(type arg_name)` (module prefix, PascalCase)
  to match the EPANET toolkit convention.
- Static/private helpers: `lowerCamelCase`, signalling internal scope.
- Descriptive identifiers (`nodeIndex`, not `n`).
- Magic numbers become named `enum` values or `const`, not bare literals.

**Structure:**

- Follow the surrounding EPANET style for indentation and braces when applying changes to EPANET.
Keep new files consistent.
- Keep functions small and single-purpose.
- Keep files focused on one module.
- Do not introduce comments unless necessary. Code should be self-descriptive through function and variable names.

## Module architecture

New LSX modules wrap their behaviour behind an opaque struct pointer and expose
only a public interface:

- `#pragma once` headers with the public function declarations.
- Constructor/destructor pairs for any heap-owned state, owned by and reachable
  from the project context.
- Descriptive accessors rather than exposing struct internals.
- Destructors that safely handle `NULL`.

Because no module may hold mutable static state, every module's constructor
takes (or is reachable from) the project context, and all its state lives in the
struct it returns.
