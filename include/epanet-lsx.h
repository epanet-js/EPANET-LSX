#pragma once
#include "epanet2_2.h"

#define MAX_LSX_ITERATIONS 10
#define LSX_WARNING 7
#define LSX_WARNING_TEXT "WARNING: Lua script raised warnings."

int LSX_open(EN_Project project, const char *inp_path);
int LSX_init(EN_Project project);
int LSX_run(EN_Project project, int *changed);
int LSX_next(EN_Project project);
int LSX_warning(EN_Project project);
int LSX_close(EN_Project project);