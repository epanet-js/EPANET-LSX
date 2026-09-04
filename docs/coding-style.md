# Coding Style

## Indentation

- Indent with two spaces.
- Don't use tabs.
- This applies to the project's own C, C++, and test (`*.test.cpp`) sources.

```cpp
describe("EPANET-LSX smoke", []() {
  it("runs a trivial assertion", [&]() {
    expect(1 + 1).toBe(2);
  });
});
```

## Braces and structure

- Use K&R style for braces, Pascal case for public function names (`ReturnType *Module_Run(void);`).
- Use camel case for private module functions (`int doSomething(int x);`).
- Use snake case for local variables and parameters (`node_index`, not `nodeIndex`).
- Keep functions small and single-purpose; keep files focused on one module.
- Prefer descriptive names over comments; add a comment only when the code
  cannot be made self-explanatory.

## Editing EPANET

When modifying the upstream EPANET sources (applied as patches under
`patches/`), follow the **surrounding EPANET style**, so the diffs stay minimal and readable.
