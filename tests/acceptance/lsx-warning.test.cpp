#include <cest>

#include <string>

#include "null-device.hpp"

extern "C" {
#include <epanet-lsx.h>
#include <epanet2_2.h>
}

static std::string fixture(const char *name) {
  return std::string(FIXTURES_DIR) + "/" + name;
}

static void captureLine(void *userData, void *projectHandle, const char *line) {
  (void)projectHandle;
  std::string *out = static_cast<std::string *>(userData);
  *out += line;
  *out += "\n";
}

static int j1Elevation(EN_Project p) {
  int index = 0;
  EN_getnodeindex(p, "J1", &index);
  double value = 0.0;
  EN_getnodevalue(p, index, EN_ELEVATION, &value);
  return (int)value;
}

describe("LSX warning()", []() {
  it("completes only the warning step with an LSX warning", [&]() {
    EN_Project p = nullptr;
    EN_createproject(&p);
    expect(EN_open(p, fixture("net-lua-warning.inp").c_str(), LSX_NULL_DEVICE, ""))
        .toBe(0);
    std::string report;
    EN_setreportcallback(p, captureLine);
    EN_setreportcallbackuserdata(p, &report);

    std::string codes;
    long t = 0, tstep = 0;
    EN_openH(p);
    EN_initH(p, EN_NOSAVE);
    do {
      int code = EN_runH(p, &t);
      codes += std::to_string(t) + ":" + std::to_string(code) + " ";
      expect(EN_nextH(p, &tstep)).toBe(0);
    } while (tstep > 0);
    EN_closeH(p);

    expect(codes).toEqual("0:0 3600:7 7200:0 ");
    expect(j1Elevation(p)).toBe(103);
    expect(report.find("   1:00:00 (Lua) WARNING: J1 elevation\t100.0\n") !=
           std::string::npos)
        .toBe(true);

    EN_close(p);
    EN_deleteproject(p);
  });

  it("makes the whole hydraulic run end with an LSX warning", [&]() {
    EN_Project p = nullptr;
    EN_createproject(&p);
    expect(EN_open(p, fixture("net-lua-warning.inp").c_str(), LSX_NULL_DEVICE, ""))
        .toBe(0);
    expect(EN_solveH(p)).toBe(LSX_WARNING);
    EN_close(p);
    EN_deleteproject(p);
  });

  it("flags a warning raised in on_hydraulics_solved on the run", [&]() {
    EN_Project p = nullptr;
    EN_createproject(&p);
    expect(EN_open(p, fixture("net-lua-warning-solved.inp").c_str(), LSX_NULL_DEVICE,
                   ""))
        .toBe(0);
    expect(EN_solveH(p)).toBe(LSX_WARNING);
    EN_close(p);
    EN_deleteproject(p);
  });

  it("describes the LSX warning code", [&]() {
    char message[EN_MAXMSG + 1] = "";
    expect(EN_geterror(LSX_WARNING, message, EN_MAXMSG)).toBe(0);
    expect(std::string(message)).toEqual(LSX_WARNING_TEXT);
  });
});
