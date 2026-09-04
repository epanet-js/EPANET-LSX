#if defined(_WIN32)
  #define DLLEXPORT __stdcall
#else
  #define DLLEXPORT
#endif

#include <stddef.h>

#include "enext.h"

static ENEXT_Handle         g_self = NULL;
static const ENEXT_HostApi *g_host = NULL;

int ENEXT_DLLEXPORT ENEXT_abi_version(void) {
  return (int)ENEXT_ABI_VERSION;
}

int ENEXT_DLLEXPORT ENEXT_load(ENEXT_Handle self, const ENEXT_HostApi *host) {
  g_self = self;
  g_host = host;
  return 0;
}

void *DLLEXPORT EN_getprivatedata(EN_Project ph) {
  return g_host->getprivatedata(g_self, ph);
}

int DLLEXPORT EN_setprivatedata(EN_Project ph, void *data) {
  return g_host->setprivatedata(g_self, ph, data);
}

#define X(ret, name, params, args) \
  ret DLLEXPORT EN_##name params { return g_host->name args; }
#include "enext_toolkit.def"
#undef X
