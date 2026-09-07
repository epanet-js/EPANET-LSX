#include <cest>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include <epanet2_2.h>
#include <lsx-errors.h>
#include <lsx-runtime.h>
}

// -- helpers ----------------------------------------------------------------

static char *dupScript(const std::string &s) {
  char *out = (char *)std::malloc(s.size() + 1);
  std::memcpy(out, s.c_str(), s.size() + 1);
  return out;
}

static std::string num(double v) {
  char b[64];
  std::snprintf(b, sizeof(b), "%.10g", v);
  return std::string(b);
}

static bool near(double a, double b) {
  return std::fabs(a - b) <= 1.0e-3 * (1.0 + std::fabs(b));
}

// A network holding one of every element the bindings can address, built
// entirely through the public toolkit. No solve is needed to read/write inputs.
static EN_Project makeNetwork() {
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

static int nodeIndex(EN_Project p, const char *id) {
  int i = 0;
  EN_getnodeindex(p, id, &i);
  return i;
}

static int linkIndex(EN_Project p, const char *id) {
  int i = 0;
  EN_getlinkindex(p, id, &i);
  return i;
}

// Round-trips one property both ways: write in Lua and read back in C (proves
// the write targets the right EN_ code), then write in C and assert in Lua
// (proves the read maps to the same code).
static void roundTripNode(const char *id, const char *prop, int code,
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

static void roundTripLink(const char *id, const char *prop, int code,
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

static void roundTripTime(const char *prop, int code, long value) {
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

// Options are read only through the binding, so validate the get path: set via
// C, read via Lua.
static void readOption(const char *prop, int code, double value) {
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

static void expectReadOnly(const std::string &lvalue) {
  EN_Project p = makeNetwork();
  LsxRuntime *rt = LsxRuntime_New(p);
  LsxRuntime_SetScript(rt, dupScript(lvalue + " = 1"));
  expect(LsxRuntime_Parse(rt)).toBe(LSX_OK);
  int changed = 0;
  expect(LsxRuntime_RunIteration(rt, &changed)).toBe(LSX_ERR_RUNTIME);
  LsxRuntime_Free(rt);
  EN_deleteproject(p);
}

// -- parameter tables -------------------------------------------------------

struct Prop {
  const char *id;
  const char *prop;
  int code;
  double value;
};

static const std::vector<Prop> kNodeProps = {
    {"J1", "elevation", EN_ELEVATION, 123.5},
    {"J1", "base_demand", EN_BASEDEMAND, 45.0},
    {"J1", "emitter", EN_EMITTER, 2.5},
    {"J1", "init_quality", EN_INITQUAL, 1.2},
    {"J1", "source_quality", EN_SOURCEQUAL, 3.4},
    {"J1", "source_type", EN_SOURCETYPE, 1.0},
    {"J1", "pattern", EN_PATTERN, 1.0},
    {"J1", "source_pattern", EN_SOURCEPAT, 1.0},
    {"T1", "elevation", EN_ELEVATION, 40.0},
    {"T1", "init_quality", EN_INITQUAL, 2.0},
    {"T1", "tank_diameter", EN_TANKDIAM, 60.0},
    {"T1", "min_volume", EN_MINVOLUME, 500.0},
    {"T1", "min_level", EN_MINLEVEL, 1.0},
    {"T1", "max_level", EN_MAXLEVEL, 90.0},
    {"T1", "tank_level", EN_TANKLEVEL, 8.0},
    {"T1", "mix_fraction", EN_MIXFRACTION, 0.5},
    {"T1", "bulk_coeff", EN_TANK_KBULK, -0.5},
    {"T1", "mix_model", EN_MIXMODEL, 1.0},
    {"T1", "can_overflow", EN_CANOVERFLOW, 1.0},
};

static const std::vector<Prop> kLinkProps = {
    {"P1", "diameter", EN_DIAMETER, 14.0},
    {"P1", "length", EN_LENGTH, 500.0},
    {"P1", "roughness", EN_ROUGHNESS, 120.0},
    {"P1", "minor_loss", EN_MINORLOSS, 0.5},
    {"P1", "bulk_coeff", EN_KBULK, -0.3},
    {"P1", "wall_coeff", EN_KWALL, -0.1},
    {"P1", "leak_area", EN_LEAK_AREA, 0.2},
    {"P1", "leak_expansion", EN_LEAK_EXPAN, 1.5},
    {"P1", "init_status", EN_INITSTATUS, 0.0},
    {"P1", "status", EN_STATUS, 0.0},
    {"PU1", "pump_power", EN_PUMP_POWER, 50.0},
};

struct TimeCase {
  const char *prop;
  int code;
  long value;
};

static const std::vector<TimeCase> kTimeProps = {
    {"duration", EN_DURATION, 86400},
    {"hydraulic_step", EN_HYDSTEP, 3600},
    {"quality_step", EN_QUALSTEP, 300},
    {"pattern_step", EN_PATTERNSTEP, 7200},
    {"pattern_start", EN_PATTERNSTART, 0},
    {"report_step", EN_REPORTSTEP, 3600},
    {"report_start", EN_REPORTSTART, 0},
    {"rule_step", EN_RULESTEP, 360},
    {"start_time", EN_STARTTIME, 0},
    {"statistic", EN_STATISTIC, 1},
};

static const std::vector<Prop> kOptionProps = {
    {nullptr, "trials", EN_TRIALS, 45.0},
    {nullptr, "accuracy", EN_ACCURACY, 0.008},
    {nullptr, "tolerance", EN_TOLERANCE, 0.03},
    {nullptr, "emitter_exponent", EN_EMITEXPON, 0.6},
    {nullptr, "demand_multiplier", EN_DEMANDMULT, 1.4},
    {nullptr, "specific_gravity", EN_SP_GRAVITY, 1.2},
    {nullptr, "specific_viscosity", EN_SP_VISCOS, 1.05},
    {nullptr, "damp_limit", EN_DAMPLIMIT, 0.1},
};

static const std::vector<std::string> kReadOnly = {
    "node('J1').pressure", "node('J1').head",   "node('J1').demand",
    "link('P1').flow",     "link('P1').velocity", "link('P1').headloss",
    "options().trials",    "options().accuracy",  "times().periods",
};

// -- tests ------------------------------------------------------------------

describe("LSX API surface", []() {
  for (const Prop c : kNodeProps) {
    it(std::string("round-trips node ") + c.id + "." + c.prop,
       [c]() { roundTripNode(c.id, c.prop, c.code, c.value); });
  }

  for (const Prop c : kLinkProps) {
    it(std::string("round-trips link ") + c.id + "." + c.prop,
       [c]() { roundTripLink(c.id, c.prop, c.code, c.value); });
  }

  for (const TimeCase c : kTimeProps) {
    it(std::string("round-trips times.") + c.prop,
       [c]() { roundTripTime(c.prop, c.code, c.value); });
  }

  for (const Prop c : kOptionProps) {
    it(std::string("reads options.") + c.prop,
       [c]() { readOption(c.prop, c.code, c.value); });
  }

  for (const std::string lvalue : kReadOnly) {
    it(std::string("rejects writing read-only ") + lvalue,
       [lvalue]() { expectReadOnly(lvalue); });
  }
});
