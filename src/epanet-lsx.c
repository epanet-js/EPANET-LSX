#include <stdio.h>
#include <stdlib.h>

#include "epanet-lsx.h"
#include "inp-parser.h"

int LSX_open(EN_Project project, const char *inp_path) {
  free(EN_getprivatedata(project));
  const char *script = InpParser_ReadScript(inp_path);
  EN_setprivatedata(project, (void *)script);
  return 0;
}

int LSX_init(EN_Project project) {
  printf("LSX_init() called\n");
  return 0;
}

int LSX_run(EN_Project project) {
  printf("LSX_run() called\n");
  return 0;
}

int LSX_next(EN_Project project) {
  printf("LSX_next() called\n");
  return 0;
}

int LSX_close(EN_Project project) {
  free(EN_getprivatedata(project));
  EN_setprivatedata(project, NULL);
  return 0;
}
