#include <math.h>
#include <stdio.h>
#include <string.h>

#include "lsx-bindings.h"
#include "lsx-errors.h"

#define LSX_READ_ONLY 0
#define LSX_WRITABLE 1
#define LSX_TINY 1.0e-6
#define NUM_API_FUNCS (sizeof(LuaApi) / sizeof(LuaApi[0]))

// A network object exposed to Lua as userdata; its metatable decides whether it
// is a node, a link or a project-wide object.
typedef struct {
  int index;
} LuaElem;

// A Lua-facing property name mapped to its EN_ property code.
typedef struct {
  const char *name;
  int code;
  int writable;
} PropDesc;

// An object type: its Lua names, its property table and the public toolkit
// functions used to look it up and to read/write its properties. A NULL find
// marks a project-wide object that takes no id; a NULL set marks one that is
// read only throughout.
typedef struct {
  const char *metatable;
  const char *global;
  const PropDesc *props;
  int (*find)(EN_Project, const char *, int *);
  int (*get)(EN_Project, int, int, double *);
  int (*set)(EN_Project, int, int, double);
} LuaApiFunc;

// Writability follows what EN_setnodevalue / EN_setlinkvalue accept.
static const PropDesc NodeProps[] = {
  { "elevation",       EN_ELEVATION,      LSX_WRITABLE  },
  { "base_demand",     EN_BASEDEMAND,     LSX_WRITABLE  },
  { "pattern",         EN_PATTERN,        LSX_WRITABLE  },
  { "emitter",         EN_EMITTER,        LSX_WRITABLE  },
  { "init_quality",    EN_INITQUAL,       LSX_WRITABLE  },
  { "source_quality",  EN_SOURCEQUAL,     LSX_WRITABLE  },
  { "source_pattern",  EN_SOURCEPAT,      LSX_WRITABLE  },
  { "source_type",     EN_SOURCETYPE,     LSX_WRITABLE  },
  { "tank_level",      EN_TANKLEVEL,      LSX_WRITABLE  },
  { "demand",          EN_DEMAND,         LSX_READ_ONLY },
  { "head",            EN_HEAD,           LSX_READ_ONLY },
  { "pressure",        EN_PRESSURE,       LSX_READ_ONLY },
  { "quality",         EN_QUALITY,        LSX_READ_ONLY },
  { "source_mass",     EN_SOURCEMASS,     LSX_READ_ONLY },
  { "init_volume",     EN_INITVOLUME,     LSX_READ_ONLY },
  { "mix_model",       EN_MIXMODEL,       LSX_WRITABLE  },
  { "mix_zone_volume", EN_MIXZONEVOL,     LSX_READ_ONLY },
  { "tank_diameter",   EN_TANKDIAM,       LSX_WRITABLE  },
  { "min_volume",      EN_MINVOLUME,      LSX_WRITABLE  },
  { "volume_curve",    EN_VOLCURVE,       LSX_WRITABLE  },
  { "min_level",       EN_MINLEVEL,       LSX_WRITABLE  },
  { "max_level",       EN_MAXLEVEL,       LSX_WRITABLE  },
  { "mix_fraction",    EN_MIXFRACTION,    LSX_WRITABLE  },
  { "bulk_coeff",      EN_TANK_KBULK,     LSX_WRITABLE  },
  { "tank_volume",     EN_TANKVOLUME,     LSX_READ_ONLY },
  { "max_volume",      EN_MAXVOLUME,      LSX_READ_ONLY },
  { "can_overflow",    EN_CANOVERFLOW,    LSX_WRITABLE  },
  { "demand_deficit",  EN_DEMANDDEFICIT,  LSX_READ_ONLY },
  { "in_control",      EN_NODE_INCONTROL, LSX_READ_ONLY },
  { "emitter_flow",    EN_EMITTERFLOW,    LSX_READ_ONLY },
  { "leakage_flow",    EN_LEAKAGEFLOW,    LSX_READ_ONLY },
  { "demand_flow",     EN_DEMANDFLOW,     LSX_READ_ONLY },
  { "full_demand",     EN_FULLDEMAND,     LSX_READ_ONLY },
  { NULL,              0,                 LSX_READ_ONLY }
};

static const PropDesc LinkProps[] = {
  { "diameter",        EN_DIAMETER,       LSX_WRITABLE  },
  { "length",          EN_LENGTH,         LSX_WRITABLE  },
  { "roughness",       EN_ROUGHNESS,      LSX_WRITABLE  },
  { "minor_loss",      EN_MINORLOSS,      LSX_WRITABLE  },
  { "init_status",     EN_INITSTATUS,     LSX_WRITABLE  },
  { "init_setting",    EN_INITSETTING,    LSX_WRITABLE  },
  { "bulk_coeff",      EN_KBULK,          LSX_WRITABLE  },
  { "wall_coeff",      EN_KWALL,          LSX_WRITABLE  },
  { "flow",            EN_FLOW,           LSX_READ_ONLY },
  { "velocity",        EN_VELOCITY,       LSX_READ_ONLY },
  { "headloss",        EN_HEADLOSS,       LSX_READ_ONLY },
  { "status",          EN_STATUS,         LSX_WRITABLE  },
  { "setting",         EN_SETTING,        LSX_WRITABLE  },
  { "energy",          EN_ENERGY,         LSX_READ_ONLY },
  { "quality",         EN_LINKQUAL,       LSX_READ_ONLY },
  { "pattern",         EN_LINKPATTERN,    LSX_WRITABLE  },
  { "pump_state",      EN_PUMP_STATE,     LSX_READ_ONLY },
  { "pump_efficiency", EN_PUMP_EFFIC,     LSX_READ_ONLY },
  { "pump_power",      EN_PUMP_POWER,     LSX_WRITABLE  },
  { "pump_hcurve",     EN_PUMP_HCURVE,    LSX_WRITABLE  },
  { "pump_ecurve",     EN_PUMP_ECURVE,    LSX_WRITABLE  },
  { "pump_ecost",      EN_PUMP_ECOST,     LSX_WRITABLE  },
  { "pump_epattern",   EN_PUMP_EPAT,      LSX_WRITABLE  },
  { "in_control",      EN_LINK_INCONTROL, LSX_READ_ONLY },
  { "gpv_curve",       EN_GPV_CURVE,      LSX_WRITABLE  },
  { "pcv_curve",       EN_PCV_CURVE,      LSX_WRITABLE  },
  { "leak_area",       EN_LEAK_AREA,      LSX_WRITABLE  },
  { "leak_expansion",  EN_LEAK_EXPAN,     LSX_WRITABLE  },
  { "leakage",         EN_LINK_LEAKAGE,   LSX_READ_ONLY },
  { "valve_type",      EN_VALVE_TYPE,     LSX_READ_ONLY },
  { NULL,              0,                 LSX_READ_ONLY }
};

static const PropDesc OptionProps[] = {
  { "trials",               EN_TRIALS,        LSX_READ_ONLY },
  { "accuracy",             EN_ACCURACY,      LSX_READ_ONLY },
  { "tolerance",            EN_TOLERANCE,     LSX_READ_ONLY },
  { "emitter_exponent",     EN_EMITEXPON,     LSX_READ_ONLY },
  { "demand_multiplier",    EN_DEMANDMULT,    LSX_READ_ONLY },
  { "head_error",           EN_HEADERROR,     LSX_READ_ONLY },
  { "flow_change",          EN_FLOWCHANGE,    LSX_READ_ONLY },
  { "headloss_form",        EN_HEADLOSSFORM,  LSX_READ_ONLY },
  { "global_efficiency",    EN_GLOBALEFFIC,   LSX_READ_ONLY },
  { "global_price",         EN_GLOBALPRICE,   LSX_READ_ONLY },
  { "global_pattern",       EN_GLOBALPATTERN, LSX_READ_ONLY },
  { "demand_charge",        EN_DEMANDCHARGE,  LSX_READ_ONLY },
  { "specific_gravity",     EN_SP_GRAVITY,    LSX_READ_ONLY },
  { "specific_viscosity",   EN_SP_VISCOS,     LSX_READ_ONLY },
  { "unbalanced",           EN_UNBALANCED,    LSX_READ_ONLY },
  { "check_frequency",      EN_CHECKFREQ,     LSX_READ_ONLY },
  { "max_check",            EN_MAXCHECK,      LSX_READ_ONLY },
  { "damp_limit",           EN_DAMPLIMIT,     LSX_READ_ONLY },
  { "specific_diffusivity", EN_SP_DIFFUS,     LSX_READ_ONLY },
  { "bulk_order",           EN_BULKORDER,     LSX_READ_ONLY },
  { "wall_order",           EN_WALLORDER,     LSX_READ_ONLY },
  { "tank_order",           EN_TANKORDER,     LSX_READ_ONLY },
  { "concentration_limit",  EN_CONCENLIMIT,   LSX_READ_ONLY },
  { "demand_pattern",       EN_DEMANDPATTERN, LSX_READ_ONLY },
  { "emitter_backflow",     EN_EMITBACKFLOW,  LSX_READ_ONLY },
  { "pressure_units",       EN_PRESS_UNITS,   LSX_READ_ONLY },
  { "status_report",        EN_STATUS_REPORT, LSX_READ_ONLY },
  { NULL,                   0,                LSX_READ_ONLY }
};

static const PropDesc TimeProps[] = {
  { "duration",             EN_DURATION,      LSX_WRITABLE  },
  { "hydraulic_step",       EN_HYDSTEP,       LSX_WRITABLE  },
  { "quality_step",         EN_QUALSTEP,      LSX_WRITABLE  },
  { "pattern_step",         EN_PATTERNSTEP,   LSX_WRITABLE  },
  { "pattern_start",        EN_PATTERNSTART,  LSX_WRITABLE  },
  { "report_step",          EN_REPORTSTEP,    LSX_WRITABLE  },
  { "report_start",         EN_REPORTSTART,   LSX_WRITABLE  },
  { "rule_step",            EN_RULESTEP,      LSX_WRITABLE  },
  { "statistic",            EN_STATISTIC,     LSX_WRITABLE  },
  { "periods",              EN_PERIODS,       LSX_READ_ONLY },
  { "start_time",           EN_STARTTIME,     LSX_WRITABLE  },
  { "hydraulic_time",       EN_HTIME,         LSX_WRITABLE  },
  { "quality_time",         EN_QTIME,         LSX_WRITABLE  },
  { "halt_flag",            EN_HALTFLAG,      LSX_READ_ONLY },
  { "next_event",           EN_NEXTEVENT,     LSX_READ_ONLY },
  { "next_event_tank",      EN_NEXTEVENTTANK, LSX_READ_ONLY },
  { NULL,                   0,                LSX_READ_ONLY }
};

static double lsxAbs(double x) {
  return x < 0.0 ? -x : x;
}

static long lsxRound(double x) {
  return (long)(x + (x >= 0.0 ? 0.5 : -0.5));
}

static int getOptionValue(EN_Project project, int index, int code,
                          double *value) {
  (void)index;
  return EN_getoption(project, code, value);
}

static int getTimeValue(EN_Project project, int index, int code,
                        double *value) {
  (void)index;
  long seconds = 0;
  int err = EN_gettimeparam(project, code, &seconds);
  *value = (double)seconds;
  return err;
}

static int setTimeValue(EN_Project project, int index, int code, double value) {
  (void)index;
  return EN_settimeparam(project, code, lsxRound(value));
}

static int getNodeIndex(EN_Project project, const char *id, int *index) {
  return EN_getnodeindex(project, id, index);
}

static int getNodeValue(EN_Project project, int index, int code, double *value) {
  return EN_getnodevalue(project, index, code, value);
}

static int setNodeValue(EN_Project project, int index, int code, double value) {
  return EN_setnodevalue(project, index, code, value);
}

static int getLinkIndex(EN_Project project, const char *id, int *index) {
  return EN_getlinkindex(project, id, index);
}

static int getLinkValue(EN_Project project, int index, int code, double *value) {
  return EN_getlinkvalue(project, index, code, value);
}

static int setLinkValue(EN_Project project, int index, int code, double value) {
  return EN_setlinkvalue(project, index, code, value);
}

static const LuaApiFunc LuaApi[] = {
  { "epanet.node",    "node",    NodeProps,   getNodeIndex, getNodeValue, setNodeValue },
  { "epanet.link",    "link",    LinkProps,   getLinkIndex, getLinkValue, setLinkValue },
  { "epanet.options", "options", OptionProps, NULL,            getOptionValue,  NULL            },
  { "epanet.times",   "times",   TimeProps,   NULL,            getTimeValue,    setTimeValue    }
};

static const PropDesc *findElementProperty(const PropDesc *props,
                                           const char *name) {
  for (; props->name != NULL; props++) {
    if (strcmp(props->name, name) == 0) return props;
  }
  return NULL;
}

static void formatClock(long seconds, char *buf, size_t size) {
  long hours = seconds / 3600;
  long minutes = (seconds % 3600) / 60;
  long secs = seconds % 60;
  snprintf(buf, size, "%ld:%02ld:%02ld", hours, minutes, secs);
}

static const char *stringOrEmpty(lua_State *lua, int arg) {
  const char *s = luaL_tolstring(lua, arg, NULL);
  if (s == NULL) return "";
  return s;
}

static int lua_epanet_print(lua_State *lua) {
  LsxRuntime *runtime = lua_touserdata(lua, lua_upvalueindex(1));
  EN_Project project = LsxRuntime_Project(runtime);
  char buf[LSX_MAX_MSG + 1];
  int nargs = lua_gettop(lua);
  int pos = 0;

  if (LsxRuntime_IsTimedEvent(runtime)) {
    long htime = 0;
    EN_gettimeparam(project, EN_HTIME, &htime);
    char clock[16];
    formatClock(htime, clock, sizeof(clock));
    pos += snprintf(buf, sizeof(buf), "%s: ", clock);
  }

  for (int i = 1; i <= nargs; i++) {
    const char *arg_as_string = stringOrEmpty(lua, i);
    if (i > 1 && pos < (int)sizeof(buf) - 1) {
      buf[pos++] = '\t';
    }

    size_t len = strlen(arg_as_string);
    if (pos + (int)len >= (int)sizeof(buf) - 1) {
      len = sizeof(buf) - 1 - pos;
    }
    memcpy(buf + pos, arg_as_string, len);
    pos += (int)len;
    lua_pop(lua, 1);
  }

  buf[pos] = '\0';
  EN_writeline(project, buf);
  return 0;
}

static int lua_curve_points(lua_State *lua) {
  LsxRuntime *runtime = lua_touserdata(lua, lua_upvalueindex(1));
  EN_Project project = LsxRuntime_Project(runtime);
  const char *id = luaL_checkstring(lua, 1);

  int index = 0;
  if (EN_getcurveindex(project, id, &index) != 0 || index == 0) {
    return luaL_error(lua, "curve not found: %s", id);
  }

  int npoints = 0;
  int err = EN_getcurvelen(project, index, &npoints);
  if (err) return luaL_error(lua, "error %d reading curve %s", err, id);

  lua_createtable(lua, npoints, 0);
  for (int i = 1; i <= npoints; i++) {
    double x, y;
    err = EN_getcurvevalue(project, index, i, &x, &y);
    if (err) {
      return luaL_error(lua, "error %d reading point %d of curve %s", err, i, id);
    }

    lua_createtable(lua, 2, 0);
    lua_pushnumber(lua, x);
    lua_rawseti(lua, -2, 1);
    lua_pushnumber(lua, y);
    lua_rawseti(lua, -2, 2);
    lua_rawseti(lua, -2, i);
  }

  return 1;
}

static int lua_elem_new(lua_State *lua) {
  LsxRuntime *runtime = lua_touserdata(lua, lua_upvalueindex(1));
  const LuaApiFunc *d = lua_touserdata(lua, lua_upvalueindex(2));
  EN_Project project = LsxRuntime_Project(runtime);
  int index = 0;

  if (d->find != NULL) {
    const char *id = luaL_checkstring(lua, 1);
    if (d->find(project, id, &index) != 0 || index == 0) {
      return luaL_error(lua, "%s not found: %s", d->global, id);
    }
  }

  LuaElem *e = lua_newuserdata(lua, sizeof(LuaElem));
  e->index = index;
  luaL_getmetatable(lua, d->metatable);
  lua_setmetatable(lua, -2);
  return 1;
}

static int lua_elem_index(lua_State *lua) {
  LsxRuntime *runtime = lua_touserdata(lua, lua_upvalueindex(1));
  const LuaApiFunc *d = lua_touserdata(lua, lua_upvalueindex(2));
  LuaElem *e = luaL_checkudata(lua, 1, d->metatable);
  EN_Project project = LsxRuntime_Project(runtime);

  const char *key = luaL_checkstring(lua, 2);
  double value;

  const PropDesc *p = findElementProperty(d->props, key);
  if (p == NULL) return luaL_error(lua, "unknown %s property: %s", d->global, key);

  int err = d->get(project, e->index, p->code, &value);
  if (err) return luaL_error(lua, "error %d reading %s.%s", err, d->global, key);

  lua_pushnumber(lua, value);
  return 1;
}

static int lua_elem_newindex(lua_State *lua) {
  LsxRuntime *runtime = lua_touserdata(lua, lua_upvalueindex(1));
  const LuaApiFunc *d = lua_touserdata(lua, lua_upvalueindex(2));
  LuaElem *e = luaL_checkudata(lua, 1, d->metatable);
  EN_Project project = LsxRuntime_Project(runtime);

  const char *key = luaL_checkstring(lua, 2);
  double value = luaL_checknumber(lua, 3);

  const PropDesc *p = findElementProperty(d->props, key);
  if (p == NULL) return luaL_error(lua, "unknown %s property: %s", d->global, key);
  if (!p->writable || d->set == NULL) {
    return luaL_error(lua, "%s property is read only: %s", d->global, key);
  }

  double before, after;
  int had_value_before = (d->get(project, e->index, p->code, &before) == 0);

  int err = d->set(project, e->index, p->code, value);
  if (err) return luaL_error(lua, "error %d writing %s.%s", err, d->global, key);

  // Flag the change so the hydraulic solver re-converges, but only if the stored
  // value actually changed: scripts re-run on every solver convergence, so no-op
  // rewrites must not keep it iterating forever.
  if (!had_value_before || d->get(project, e->index, p->code, &after) != 0 ||
      lsxAbs(after - before) > LSX_TINY * (1.0 + lsxAbs(before))) {
    LsxRuntime_MarkChanged(runtime);
  }
  return 0;
}

static void registerFunction(lua_State *lua, const LuaApiFunc *fn,
                             LsxRuntime *runtime) {
  luaL_newmetatable(lua, fn->metatable);
  lua_pushlightuserdata(lua, runtime);
  lua_pushlightuserdata(lua, (void *)fn);
  lua_pushcclosure(lua, lua_elem_index, 2);
  lua_setfield(lua, -2, "__index");

  lua_pushlightuserdata(lua, runtime);
  lua_pushlightuserdata(lua, (void *)fn);
  lua_pushcclosure(lua, lua_elem_newindex, 2);
  lua_setfield(lua, -2, "__newindex");
  lua_pop(lua, 1);

  lua_pushlightuserdata(lua, runtime);
  lua_pushlightuserdata(lua, (void *)fn);
  lua_pushcclosure(lua, lua_elem_new, 2);
  lua_setglobal(lua, fn->global);
}

void LsxBindings_Register(lua_State *lua, LsxRuntime *runtime) {
  lua_pushlightuserdata(lua, runtime);
  lua_pushcclosure(lua, lua_epanet_print, 1);
  lua_setglobal(lua, "print");

  lua_pushlightuserdata(lua, runtime);
  lua_pushcclosure(lua, lua_curve_points, 1);
  lua_setglobal(lua, "curve");

  for (size_t i = 0; i < NUM_API_FUNCS; i++) {
    registerFunction(lua, &LuaApi[i], runtime);
  }
}
