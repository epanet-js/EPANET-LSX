#include <cest>

#include <string>
#include <vector>

#include "api-surface-helpers.hpp"

using namespace TestHelpers;

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
