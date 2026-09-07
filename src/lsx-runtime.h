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

int LsxRuntime_SetScript(LsxRuntime *runtime, char *script);
const char *LsxRuntime_GetScript(const LsxRuntime *runtime);

int LsxRuntime_Parse(LsxRuntime *runtime);

int LsxRuntime_RunIteration(LsxRuntime *runtime, int *changed);

EN_Project LsxRuntime_Project(const LsxRuntime *runtime);
lua_State *LsxRuntime_State(const LsxRuntime *runtime);
void LsxRuntime_MarkChanged(LsxRuntime *runtime);
void LsxRuntime_ResetChanged(LsxRuntime *runtime);
int LsxRuntime_Changed(const LsxRuntime *runtime);
void LsxRuntime_SetTimedEvent(LsxRuntime *runtime, int on);
int LsxRuntime_IsTimedEvent(const LsxRuntime *runtime);
