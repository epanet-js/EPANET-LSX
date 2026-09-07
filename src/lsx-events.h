#pragma once

#include "lsx-runtime.h"

typedef enum {
  LSX_EVENT_OPEN,
  LSX_EVENT_CLOSE,
  LSX_EVENT_HYDRAULICS_SOLVED,
  LSX_EVENT_HYDRAULIC_STEP,
  LSX_EVENT_MAX
} LsxEvent;

int LsxEvents_Dispatch(LsxRuntime *runtime, LsxEvent event, int *changed);
