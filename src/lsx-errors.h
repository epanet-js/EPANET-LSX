#pragma once

// LSX error codes. 101 mirrors EPANET's insufficient-memory code; the 31x range
// is LSX-specific and propagates out through the EN_*Q/EN_*H return values.
enum {
  LSX_OK = 0,
  LSX_ERR_MEMORY = 101,
  LSX_ERR_STATE = 310,
  LSX_ERR_NO_ENGINE = 311,
  LSX_ERR_LOAD = 312,
  LSX_ERR_RUNTIME = 313
};

#define LSX_MAX_MSG 255
