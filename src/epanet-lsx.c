#include <stdlib.h>

#include "epanet-lsx.h"
#include "inp-parser.h"
#include "lsx-runtime.h"
#include "lsx-events.h"
#include "lsx-errors.h"

int LSX_open(EN_Project project, const char *inp_path) {
  LsxRuntime_Free((LsxRuntime *)EN_getprivatedata(project));
  EN_setprivatedata(project, NULL);

  char *script = InpParser_ReadScript(inp_path);
  if (script == NULL) return LSX_OK;

  LsxRuntime *runtime = LsxRuntime_New(project);
  if (runtime == NULL) {
    free(script);
    return LSX_ERR_MEMORY;
  }

  int err = LsxRuntime_SetScript(runtime, script);
  if (!err) err = LsxRuntime_Parse(runtime);
  if (err) {
    LsxRuntime_Free(runtime);
    return err;
  }

  EN_setprivatedata(project, runtime);
  return LSX_OK;
}

int LSX_init(EN_Project project) {
  LsxRuntime *runtime = EN_getprivatedata(project);
  if (runtime == NULL) return LSX_OK;
  return LsxEvents_Dispatch(runtime, LSX_EVENT_OPEN, NULL);
}

int LSX_run(EN_Project project, int *changed) {
  if (changed != NULL) *changed = 0;
  LsxRuntime *runtime = EN_getprivatedata(project);
  if (runtime == NULL) return LSX_OK;
  return LsxRuntime_RunIteration(runtime, changed);
}

int LSX_next(EN_Project project) {
  LsxRuntime *runtime = EN_getprivatedata(project);
  if (runtime == NULL) return LSX_OK;
  return LsxEvents_Dispatch(runtime, LSX_EVENT_HYDRAULICS_SOLVED, NULL);
}

int LSX_close(EN_Project project) {
  LsxRuntime *runtime = EN_getprivatedata(project);
  if (runtime == NULL) return LSX_OK;

  LsxEvents_Dispatch(runtime, LSX_EVENT_CLOSE, NULL);
  LsxRuntime_Free(runtime);
  EN_setprivatedata(project, NULL);
  return LSX_OK;
}
