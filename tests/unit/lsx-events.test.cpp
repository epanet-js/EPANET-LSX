#include <cest>

#include <string>

#include "test-helpers.hpp"

extern "C" {
#include <lsx-errors.h>
#include <lsx-events.h>
#include <lsx-runtime.h>
}

using namespace TestHelpers;

static const char *kHandlers =
    "function on_open() node('J1').elevation = 1 end\n"
    "function on_close() node('J1').elevation = 2 end\n"
    "function on_hydraulics_solved() node('J1').elevation = 3 end\n"
    "function on_hydraulic_step() node('J1').elevation = 4 end\n";

describe("LsxEvents", []() {
  struct Mapping {
    LsxEvent event;
    int marker;
  };
  it("dispatches each event to its matching handler", [&]() {
    cest::withParameter<Mapping>()
        .withValue({LSX_EVENT_OPEN, 1})
        .withValue({LSX_EVENT_CLOSE, 2})
        .withValue({LSX_EVENT_HYDRAULICS_SOLVED, 3})
        .withValue({LSX_EVENT_HYDRAULIC_STEP, 4})
        .thenDo([](Mapping m) {
          EN_Project p = makeProject();
          LsxRuntime *rt = LsxRuntime_New(p);
          LsxRuntime_SetScript(rt, dupScript(kHandlers));
          expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);

          expect(LsxEvents_Dispatch(rt, m.event, nullptr)).toBe(LSX_OK);
          expect((int)elevationOf(p)).toBe(m.marker);

          LsxRuntime_Free(rt);
          EN_deleteproject(p);
        });
  });

  it("is a no-op when the event has no handler", [&]() {
    EN_Project p = makeProject();
    LsxRuntime *rt = LsxRuntime_New(p);
    LsxRuntime_SetScript(rt, dupScript("local unused = 1"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);

    int changed = 7;
    expect(LsxEvents_Dispatch(rt, LSX_EVENT_OPEN, &changed)).toBe(LSX_OK);
    expect(changed).toBe(0);
    expect((int)elevationOf(p)).toBe(100);

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

  it("returns a runtime error and reports it when a handler throws", [&]() {
    std::string report;
    EN_Project p = makeProject(&report);
    LsxRuntime *rt = LsxRuntime_New(p);
    LsxRuntime_SetScript(rt, dupScript("function on_open() error('boom') end"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);

    expect(LsxEvents_Dispatch(rt, LSX_EVENT_OPEN, nullptr)).toBe(LSX_ERR_RUNTIME);
    expect(report.find("LSX on_open error") != std::string::npos).toBe(true);

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

  it("reports changed when a handler changes the network", [&]() {
    EN_Project p = makeProject();
    LsxRuntime *rt = LsxRuntime_New(p);
    LsxRuntime_SetScript(rt, dupScript("function on_hydraulic_step() "
                                       "node('J1').elevation = 200 end"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);

    int changed = 0;
    expect(LsxEvents_Dispatch(rt, LSX_EVENT_HYDRAULIC_STEP, &changed)).toBe(LSX_OK);
    expect(changed).toBe(1);

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

  it("reports no change when a handler touches nothing", [&]() {
    EN_Project p = makeProject();
    LsxRuntime *rt = LsxRuntime_New(p);
    LsxRuntime_SetScript(rt, dupScript("function on_open() local x = 1 end"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);

    int changed = 1;
    expect(LsxEvents_Dispatch(rt, LSX_EVENT_OPEN, &changed)).toBe(LSX_OK);
    expect(changed).toBe(0);

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });

  it("timestamps print() only for timed events", [&]() {
    std::string report;
    EN_Project p = makeProject(&report);
    LsxRuntime *rt = LsxRuntime_New(p);
    LsxRuntime_SetScript(rt,
                         dupScript("function on_open() print('hi') end\n"
                                   "function on_hydraulic_step() print('hi') end\n"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);

    expect(LsxEvents_Dispatch(rt, LSX_EVENT_OPEN, nullptr)).toBe(LSX_OK);
    expect(report).toEqual("(Lua) hi\n");

    report.clear();
    expect(LsxEvents_Dispatch(rt, LSX_EVENT_HYDRAULIC_STEP, nullptr)).toBe(LSX_OK);
    expect(report).toEqual("   0:00:00 (Lua) hi\n");

    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  });
});
