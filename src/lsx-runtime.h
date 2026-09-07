#pragma once

#include "epanet2_2.h"
#include "minilua.h"

// Per-project Lua runtime. Opaque: all state lives in the struct reachable from
// the project context (via EN_set/getprivatedata), never in module statics, so
// concurrent projects stay independent.
typedef struct LsxRuntime LsxRuntime;

// Constructor/destructor. LsxRuntime_Free accepts NULL.
LsxRuntime *LsxRuntime_New(EN_Project project);
void LsxRuntime_Free(LsxRuntime *runtime);

// Hands ownership of a heap-allocated script string to the runtime.
int LsxRuntime_SetScript(LsxRuntime *runtime, char *script);
const char *LsxRuntime_GetScript(const LsxRuntime *runtime);

// Compiles the script and evaluates its top-level chunk once. A load error is
// returned; a runtime error in the load-time evaluation is reported but left to
// surface on the first iteration.
int LsxRuntime_Parse(LsxRuntime *runtime);

// Runs one solver iteration of the script: the on_hydraulic_step handler when
// the script defines one, otherwise the top-level chunk. Sets *changed to
// whether the script wrote a network property whose value actually changed.
int LsxRuntime_RunIteration(LsxRuntime *runtime, int *changed);

// Accessors used by the bindings and event modules.
EN_Project LsxRuntime_Project(const LsxRuntime *runtime);
lua_State *LsxRuntime_State(const LsxRuntime *runtime);
void LsxRuntime_MarkChanged(LsxRuntime *runtime);
void LsxRuntime_ResetChanged(LsxRuntime *runtime);
int LsxRuntime_Changed(const LsxRuntime *runtime);
void LsxRuntime_SetTimedEvent(LsxRuntime *runtime, int on);
int LsxRuntime_IsTimedEvent(const LsxRuntime *runtime);
