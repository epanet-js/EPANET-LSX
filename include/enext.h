#pragma once
#include "epanet2_2.h"

#define ENEXT_ABI_VERSION 1u

#ifndef ENEXT_DLLEXPORT
  #ifdef _WIN32
    #ifdef ENEXT_PLUGIN_EXPORTS
      #define ENEXT_DLLEXPORT __declspec(dllexport) __stdcall
    #else
      #define ENEXT_DLLEXPORT __declspec(dllimport) __stdcall
    #endif
  #elif defined(__CYGWIN__)
    #define ENEXT_DLLEXPORT __stdcall
  #else
    #define ENEXT_DLLEXPORT
  #endif
#endif

#ifndef ENEXT_CALL
  #if defined(_WIN32) || defined(__CYGWIN__)
    #define ENEXT_CALL __stdcall
  #else
    #define ENEXT_CALL
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void *ENEXT_Handle;

typedef struct ENEXT_HostApi {
  unsigned int api_version;
  unsigned int struct_size;

  void *(ENEXT_CALL *getprivatedata)(ENEXT_Handle self, EN_Project ph);
  int   (ENEXT_CALL *setprivatedata)(ENEXT_Handle self, EN_Project ph, void *data);

  #define X(ret, name, params, args) ret (ENEXT_CALL *name) params;
  #include "enext_toolkit.def"
  #undef X
} ENEXT_HostApi;

int ENEXT_DLLEXPORT ENEXT_abi_version(void);
int ENEXT_DLLEXPORT ENEXT_load(ENEXT_Handle self, const ENEXT_HostApi *host);
int ENEXT_DLLEXPORT ENEXT_open(EN_Project ph, const char *inpFile);
int ENEXT_DLLEXPORT ENEXT_init(EN_Project ph);
int ENEXT_DLLEXPORT ENEXT_run(EN_Project ph);
int ENEXT_DLLEXPORT ENEXT_next(EN_Project ph);
int ENEXT_DLLEXPORT ENEXT_close(EN_Project ph);

#define ENEXT_SYM_ABI_VERSION "ENEXT_abi_version"
#define ENEXT_SYM_LOAD        "ENEXT_load"
#define ENEXT_SYM_OPEN        "ENEXT_open"
#define ENEXT_SYM_INIT        "ENEXT_init"
#define ENEXT_SYM_RUN         "ENEXT_run"
#define ENEXT_SYM_NEXT        "ENEXT_next"
#define ENEXT_SYM_CLOSE       "ENEXT_close"

#ifdef __cplusplus
}
#endif
