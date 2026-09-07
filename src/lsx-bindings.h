#pragma once

#include "lsx-runtime.h"

// Registers the epanet Lua API (print, curve, node/link/options/times) on a Lua
// state, bound to a project's runtime.
void LsxBindings_Register(lua_State *state, LsxRuntime *runtime);
