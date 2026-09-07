#include <cest>

#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include <epanet2_2.h>
#include <lsx-errors.h>
#include <lsx-runtime.h>
}

static char *dupScript(const char *s) {
  size_t n = std::strlen(s) + 1;
  char *out = (char *)std::malloc(n);
  std::memcpy(out, s, n);
  return out;
}

// A one-junction project built entirely through the public toolkit, enough for
// the bindings to look up and read/write a node without a hydraulic solve.
static EN_Project makeProject() {
  EN_Project p = nullptr;
  EN_createproject(&p);
  EN_init(p, "/dev/null", "", EN_GPM, EN_HW);
  int index = 0;
  EN_addnode(p, "J1", EN_JUNCTION, &index);
  EN_setnodevalue(p, index, EN_ELEVATION, 100.0);
  return p;
}

static double elevationOf(EN_Project p) {
  double value = 0.0;
  EN_getnodevalue(p, 1, EN_ELEVATION, &value);
  return value;
}

describe("LsxRuntime", []() {
  it("frees a NULL runtime harmlessly", [&]() { LsxRuntime_Free(nullptr); });

  it("creates and frees a runtime", [&]() {
    EN_Project p = makeProject();
    LsxRuntime *rt = LsxRuntime_New(p);

    expect(rt).toBeNotNull();

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

  it("parses and runs a trivial script", [&]() {
    EN_Project p = makeProject();
    LsxRuntime *rt = LsxRuntime_New(p);

    expect(LsxRuntime_SetScript(rt, dupScript("local x = 1 + 1"))).toBe(LSX_OK);
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);

    int changed = -1;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_OK);
    expect(changed).toBe(0);

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

  it("reports a load error for invalid syntax", [&]() {
    EN_Project p = makeProject();
    LsxRuntime *rt = LsxRuntime_New(p);

    LsxRuntime_SetScript(rt, dupScript("this is not (( lua"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_ERR_LOAD);

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

  it("defers a load-time runtime error to the iteration", [&]() {
    EN_Project p = makeProject();
    LsxRuntime *rt = LsxRuntime_New(p);

    LsxRuntime_SetScript(rt, dupScript("error('boom')"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);

    int changed = 0;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_ERR_RUNTIME);

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

  it("flags a change when a writable value actually changes", [&]() {
    EN_Project p = makeProject();
    LsxRuntime *rt = LsxRuntime_New(p);

    LsxRuntime_SetScript(rt, dupScript("node('J1').elevation = node('J1').elevation + 10"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);
    expect((int)elevationOf(p)).toBe(110);  // load-time evaluation applied once

    int changed = 0;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_OK);
    expect(changed).toBe(1);
    expect((int)elevationOf(p)).toBe(120);

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

  it("does not flag a no-op rewrite", [&]() {
    EN_Project p = makeProject();
    LsxRuntime *rt = LsxRuntime_New(p);

    LsxRuntime_SetScript(rt, dupScript("node('J1').elevation = 100"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);

    int changed = 1;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_OK);
    expect(changed).toBe(0);

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

  it("rejects a write to a read-only property", [&]() {
    EN_Project p = makeProject();
    LsxRuntime *rt = LsxRuntime_New(p);

    LsxRuntime_SetScript(rt, dupScript("node('J1').pressure = 5"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);

    int changed = 0;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_ERR_RUNTIME);

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

  it("keeps two concurrent runtimes independent", [&]() {
    EN_Project p1 = makeProject();
    EN_Project p2 = makeProject();
    LsxRuntime *rt1 = LsxRuntime_New(p1);
    LsxRuntime *rt2 = LsxRuntime_New(p2);

    LsxRuntime_SetScript(rt1, dupScript("shared = 5"));
    LsxRuntime_SetScript(rt2, dupScript("assert(shared == nil, 'state leaked between runtimes')"));
    expect(LsxRuntime_Parse(rt1)).toBe(LSX_OK);
    expect(LsxRuntime_Parse(rt2)).toBe(LSX_OK);

    int changed = 0;
    expect(LsxRuntime_RunIteration(rt1, &changed)).toBe(LSX_OK);
    expect(LsxRuntime_RunIteration(rt2, &changed)).toBe(LSX_OK);

    LsxRuntime_Free(rt1);
    LsxRuntime_Free(rt2);
    EN_deleteproject(p1);
    EN_deleteproject(p2);
  });
});
