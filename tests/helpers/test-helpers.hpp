#pragma once

#include <cstdlib>
#include <cstring>
#include <string>

#include "null-device.hpp"

extern "C" {
#include <epanet2_2.h>
}

namespace TestHelpers {

inline char *dupScript(const std::string &s) {
  char *out = (char *)std::malloc(s.size() + 1);
  std::memcpy(out, s.c_str(), s.size() + 1);
  return out;
}

inline void captureLine(void *userData, void *projectHandle, const char *line) {
  (void)projectHandle;
  std::string *out = static_cast<std::string *>(userData);
  *out += line;
  *out += "\n";
}

inline EN_Project makeProject(std::string *capture = nullptr) {
  EN_Project p = nullptr;
  EN_createproject(&p);
  EN_init(p, LSX_NULL_DEVICE, "", EN_GPM, EN_HW);
  int index = 0;
  EN_addnode(p, "J1", EN_JUNCTION, &index);
  EN_setnodevalue(p, index, EN_ELEVATION, 100.0);
  if (capture != nullptr) {
    EN_setreportcallback(p, captureLine);
    EN_setreportcallbackuserdata(p, capture);
  }
  return p;
}

inline double elevationOf(EN_Project p) {
  double value = 0.0;
  EN_getnodevalue(p, 1, EN_ELEVATION, &value);
  return value;
}

}
