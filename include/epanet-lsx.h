#pragma once
#include "epanet2_2.h"

int LSX_open(EN_Project project, const char *inp_path);
int LSX_init(EN_Project project);
int LSX_run(EN_Project project);
int LSX_next(EN_Project project);
int LSX_close(EN_Project project);