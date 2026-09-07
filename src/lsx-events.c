#include <stdio.h>

#include "lsx-events.h"
#include "lsx-errors.h"

static const char *event_name[LSX_EVENT_MAX] = {
  "on_open",
  "on_close",
  "on_hydraulics_solved",
  "on_hydraulic_step"
};

int LsxEvents_Dispatch(LsxRuntime *runtime, LsxEvent event, int *changed) {
  if (changed != NULL) *changed = 0;

  lua_State *state = LsxRuntime_State(runtime);
  if (state == NULL || LsxRuntime_GetScript(runtime) == NULL) return LSX_OK;

  LsxRuntime_ResetChanged(runtime);

  lua_getglobal(state, event_name[event]);
  if (!lua_isfunction(state, -1)) {
    lua_pop(state, 1);
    return LSX_OK;
  }

  int timed = (event == LSX_EVENT_HYDRAULIC_STEP ||
               event == LSX_EVENT_HYDRAULICS_SOLVED);
  LsxRuntime_SetTimedEvent(runtime, timed);
  int result = lua_pcall(state, 0, 0, 0);
  LsxRuntime_SetTimedEvent(runtime, 0);

  if (result != LUA_OK) {
    char msg[LSX_MAX_MSG + 1];
    snprintf(msg, sizeof(msg), "LSX %s error: %s", event_name[event],
             lua_tostring(state, -1));
    EN_writeline(LsxRuntime_Project(runtime), msg);
    lua_pop(state, 1);
    return LSX_ERR_RUNTIME;
  }

  if (changed != NULL) *changed = LsxRuntime_Changed(runtime);
  return LSX_OK;
}
