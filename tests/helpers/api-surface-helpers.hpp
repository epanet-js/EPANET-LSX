#pragma once

#include <cest>

#include <cmath>
#include <cstdio>
#include <string>

#include "test-helpers.hpp"

extern "C" {
#include <lsx-errors.h>
#include <lsx-runtime.h>
}

namespace TestHelpers {

inline std::string num(double v) {
  char b[64];
  std::snprintf(b, sizeof(b), "%.10g", v);
  return std::string(b);
}

inline bool near(double a, double b) {
  return std::fabs(a - b) <= 1.0e-3 * (1.0 + std::fabs(b));
}

inline EN_Project makeNetwork() {
  EN_Project p = nullptr;
  EN_createproject(&p);
  EN_init(p, "/dev/null", "", EN_GPM, EN_HW);

  int i = 0;
  EN_addnode(p, "R1", EN_RESERVOIR, &i);
  EN_addnode(p, "J1", EN_JUNCTION, &i);
  EN_addnode(p, "J2", EN_JUNCTION, &i);
  EN_addnode(p, "T1", EN_TANK, &i);
  EN_setnodevalue(p, i, EN_ELEVATION, 0.0);
  EN_setnodevalue(p, i, EN_TANKDIAM, 50.0);
  EN_setnodevalue(p, i, EN_MINLEVEL, 0.0);
  EN_setnodevalue(p, i, EN_MAXLEVEL, 100.0);
  EN_setnodevalue(p, i, EN_TANKLEVEL, 10.0);

  int l = 0;
  EN_addlink(p, "P1", EN_PIPE, "R1", "J1", &l);
  EN_addlink(p, "PU1", EN_PUMP, "J1", "J2", &l);
  EN_addpattern(p, "PAT1");
  EN_addcurve(p, "CV1");
  return p;
}

inline int nodeIndex(EN_Project p, const char *id) {
  int i = 0;
  EN_getnodeindex(p, id, &i);
  return i;
}

inline int linkIndex(EN_Project p, const char *id) {
  int i = 0;
  EN_getlinkindex(p, id, &i);
  return i;
}

inline void roundTripNode(const char *id, const char *prop, int code,
                          double value) {
  {
    EN_Project p = makeNetwork();
    LsxRuntime *rt = LsxRuntime_New(p);
    LsxRuntime_SetScript(rt, dupScript(std::string("node('") + id + "')." +
                                       prop + " = " + num(value)));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);
    int changed = 0;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_OK);

    double read = 0.0;
    expect(EN_getnodevalue(p, nodeIndex(p, id), code, &read)).toBe(0);
    expect(near(read, value)).toBe(true);
    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  }
  {
    EN_Project p = makeNetwork();
    LsxRuntime *rt = LsxRuntime_New(p);
    expect(EN_setnodevalue(p, nodeIndex(p, id), code, value)).toBe(0);
    LsxRuntime_SetScript(
        rt, dupScript(std::string("assert(math.abs(node('") + id + "')." + prop +
                      " - " + num(value) + ") <= 1e-3 * (1 + math.abs(" +
                      num(value) + ")), '" + prop + "')"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);
    int changed = 0;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_OK);
    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  }
}

inline void roundTripLink(const char *id, const char *prop, int code,
                          double value) {
  {
    EN_Project p = makeNetwork();
    LsxRuntime *rt = LsxRuntime_New(p);
    LsxRuntime_SetScript(rt, dupScript(std::string("link('") + id + "')." +
                                       prop + " = " + num(value)));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);
    int changed = 0;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_OK);

    double read = 0.0;
    expect(EN_getlinkvalue(p, linkIndex(p, id), code, &read)).toBe(0);
    expect(near(read, value)).toBe(true);
    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  }
  {
    EN_Project p = makeNetwork();
    LsxRuntime *rt = LsxRuntime_New(p);
    expect(EN_setlinkvalue(p, linkIndex(p, id), code, value)).toBe(0);
    LsxRuntime_SetScript(
        rt, dupScript(std::string("assert(math.abs(link('") + id + "')." + prop +
                      " - " + num(value) + ") <= 1e-3 * (1 + math.abs(" +
                      num(value) + ")), '" + prop + "')"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);
    int changed = 0;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_OK);
    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  }
}

inline void roundTripTime(const char *prop, int code, long value) {
  {
    EN_Project p = makeNetwork();
    LsxRuntime *rt = LsxRuntime_New(p);
    LsxRuntime_SetScript(rt, dupScript(std::string("times().") + prop + " = " +
                                       std::to_string(value)));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);
    int changed = 0;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_OK);

    long read = 0;
    expect(EN_gettimeparam(p, code, &read)).toBe(0);
    expect(read).toBe(value);
    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  }
  {
    EN_Project p = makeNetwork();
    LsxRuntime *rt = LsxRuntime_New(p);
    expect(EN_settimeparam(p, code, value)).toBe(0);
    LsxRuntime_SetScript(rt, dupScript(std::string("assert(times().") + prop +
                                       " == " + std::to_string(value) + ", '" +
                                       prop + "')"));
    expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);
    int changed = 0;
    expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_OK);
    LsxRuntime_Free(rt);
    EN_deleteproject(p);
  }
}

inline void readOption(const char *prop, int code, double value) {
  EN_Project p = makeNetwork();
  LsxRuntime *rt = LsxRuntime_New(p);
  expect(EN_setoption(p, code, value)).toBe(0);
  LsxRuntime_SetScript(
      rt, dupScript(std::string("assert(math.abs(options().") + prop + " - " +
                    num(value) + ") <= 1e-3 * (1 + math.abs(" + num(value) +
                    ")), '" + prop + "')"));
  expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);
  int changed = 0;
  expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_OK);
  LsxRuntime_Free(rt);
  EN_deleteproject(p);
}

inline void expectReadOnly(const std::string &lvalue) {
  EN_Project p = makeNetwork();
  LsxRuntime *rt = LsxRuntime_New(p);
  LsxRuntime_SetScript(rt, dupScript(lvalue + " = 1"));
  expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);
  int changed = 0;
  expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_ERR_RUNTIME);
  LsxRuntime_Free(rt);
  EN_deleteproject(p);
}

}
