#include <stdio.h>
#include <stdlib.h>

#include "lsx-runtime.h"
#include "lsx-bindings.h"
#include "lsx-events.h"
#include "lsx-errors.h"

struct LsxRuntime {
  EN_Project project;
  lua_State *state;
  char *script;
  int changed;
  int global_closure_ref;
  int timed_event;
};

LsxRuntime *LsxRuntime_New(EN_Project project) {
  LsxRuntime *runtime = calloc(1, sizeof(LsxRuntime));
  if (runtime == NULL) return NULL;

  runtime->project = project;
  runtime->global_closure_ref = LUA_NOREF;

  runtime->state = luaL_newstate();
  if (runtime->state == NULL) {
    free(runtime);
    return NULL;
  }

  luaL_openlibs(runtime->state);
  LsxBindings_Register(runtime->state, runtime);
  return runtime;
}

void LsxRuntime_Free(LsxRuntime *runtime) {
  if (runtime == NULL) return;

  if (runtime->state != NULL) {
    if (runtime->global_closure_ref != LUA_NOREF) {
      luaL_unref(runtime->state, LUA_REGISTRYINDEX, runtime->global_closure_ref);
    }
    lua_close(runtime->state);
  }
  free(runtime->script);
  free(runtime);
}

int LsxRuntime_SetScript(LsxRuntime *runtime, char *script) {
  if (runtime == NULL) return LSX_ERR_NO_ENGINE;
  free(runtime->script);
  runtime->script = script;
  return LSX_OK;
}

const char *LsxRuntime_GetScript(const LsxRuntime *runtime) {
  if (runtime == NULL) return NULL;
  return runtime->script;
}

static void reportError(LsxRuntime *runtime, const char *prefix) {
  char msg[LSX_MAX_MSG + 1];
  snprintf(msg, sizeof(msg), "%s%s", prefix,
           lua_tostring(runtime->state, -1));
  EN_writeline(runtime->project, msg);
}

static int runChunk(LsxRuntime *runtime, int *changed) {
  if (changed != NULL) *changed = 0;

  if (runtime->state == NULL ||
      runtime->global_closure_ref == LUA_NOREF) {
    return LSX_OK;
  }

  runtime->changed = 0;

  lua_rawgeti(runtime->state, LUA_REGISTRYINDEX, runtime->global_closure_ref);
  if (lua_pcall(runtime->state, 0, 0, 0) != LUA_OK) {
    reportError(runtime, "LSX script error: ");
    lua_pop(runtime->state, 1);
    return LSX_ERR_RUNTIME;
  }

  if (changed != NULL) *changed = runtime->changed;
  return LSX_OK;
}

int LsxRuntime_Parse(LsxRuntime *runtime) {
  if (runtime == NULL || runtime->state == NULL) return LSX_ERR_NO_ENGINE;
  if (runtime->script == NULL) return LSX_OK;

  if (luaL_loadstring(runtime->state, runtime->script) != LUA_OK) {
    reportError(runtime, "LSX load error: ");
    lua_pop(runtime->state, 1);
    return LSX_ERR_LOAD;
  }
  runtime->global_closure_ref = luaL_ref(runtime->state, LUA_REGISTRYINDEX);

  // The load-time evaluation runs against a network that has not been solved
  // yet, so a runtime error here is reported but left to be raised again by the
  // first iteration proper.
  runChunk(runtime, NULL);
  runtime->changed = 0;
  return LSX_OK;
}

int LsxRuntime_RunIteration(LsxRuntime *runtime, int *changed) {
  if (changed != NULL) *changed = 0;
  if (runtime == NULL || runtime->state == NULL) return LSX_OK;

  lua_getglobal(runtime->state, "on_hydraulic_step");
  int has_handler = lua_isfunction(runtime->state, -1);
  lua_pop(runtime->state, 1);

  if (has_handler) {
    return LsxEvents_Dispatch(runtime, LSX_EVENT_HYDRAULIC_STEP, changed);
  }
  return runChunk(runtime, changed);
}

EN_Project LsxRuntime_Project(const LsxRuntime *runtime) {
  return runtime->project;
}

lua_State *LsxRuntime_State(const LsxRuntime *runtime) {
  return runtime->state;
}

void LsxRuntime_MarkChanged(LsxRuntime *runtime) {
  if (runtime != NULL) runtime->changed = 1;
}

void LsxRuntime_ResetChanged(LsxRuntime *runtime) {
  if (runtime != NULL) runtime->changed = 0;
}

int LsxRuntime_Changed(const LsxRuntime *runtime) {
  return runtime->changed;
}

void LsxRuntime_SetTimedEvent(LsxRuntime *runtime, int on) {
  if (runtime != NULL) runtime->timed_event = on;
}

int LsxRuntime_IsTimedEvent(const LsxRuntime *runtime) {
  return runtime->timed_event;
}
