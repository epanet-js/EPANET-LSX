#include <cest>

#include <string>

#include "test-helpers.hpp"

extern "C" {
#include <lsx-errors.h>
#include <lsx-runtime.h>
}

using namespace TestHelpers;

describe("LsxRuntime", []() {
  it("frees a NULL runtime harmlessly", [&]() { LsxRuntime_Free(nullptr); });

  it("timestamps the global script body run each hydraulic step", [&]() {
    std::string report;
    EN_Project p = makeProject(&report);
    LsxRuntime *rt = LsxRuntime_New(p);
    LsxRuntime_SetScript(rt, dupScript("print('hi')"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);

    report.clear();
    int changed = 0;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_OK);
    expect(report).toEqual("   0:00:00 (Lua) hi\n");

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

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
    expect((int)elevationOf(p)).toBe(110);

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
