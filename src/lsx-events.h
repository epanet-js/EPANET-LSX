#pragma once

#include "lsx-runtime.h"

typedef enum {
  LSX_EVENT_OPEN,
  LSX_EVENT_CLOSE,
  LSX_EVENT_HYDRAULICS_SOLVED,
  LSX_EVENT_HYDRAULIC_STEP,
  LSX_EVENT_MAX
} LsxEvent;

// Calls the script's handler for an event when it defines one; a missing
// handler is a no-op. Sets *changed to whether the handler wrote a network
// property whose value actually changed.
int LsxEvents_Dispatch(LsxRuntime *runtime, LsxEvent event, int *changed);
