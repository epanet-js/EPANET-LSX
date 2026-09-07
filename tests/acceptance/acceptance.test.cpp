#include <cest>

#include <string>

extern "C" {
#include <epanet-lsx.h>
#include <epanet2_2.h>
}

#ifndef FIXTURES_DIR
#define FIXTURES_DIR "."
#endif

static const char *kReportSink = "/dev/null";

static std::string fixture(const char *name) {
  return std::string(FIXTURES_DIR) + "/" + name;
}

static void captureLine(void *userData, void *projectHandle, const char *line) {
  (void)projectHandle;
  std::string *out = static_cast<std::string *>(userData);
  *out += line;
  *out += "\n";
}

static void captureInto(EN_Project p, std::string *out) {
  EN_setreportcallback(p, captureLine);
  EN_setreportcallbackuserdata(p, out);
}

describe("LSX acceptance", []() {
  it("runs a scripted model and routes print() to the report", [&]() {
    EN_Project p = nullptr;
    EN_createproject(&p);
    expect(EN_open(p, fixture("net-with-lua.inp").c_str(), kReportSink, ""))
        .toBe(0);
    expect(EN_getprivatedata(p)).toBeNotNull();

    std::string report;
    captureInto(p, &report);
    expect(EN_solveH(p)).toBeLessThan(100);
    EN_close(p);
    EN_deleteproject(p);

    expect(report.find("value is 42") != std::string::npos).toBe(true);
  });

  it("applies a script's write to the network", [&]() {
    EN_Project p = nullptr;
    EN_createproject(&p);
    expect(EN_open(p, fixture("net-lua-modify.inp").c_str(), kReportSink, ""))
        .toBe(0);
    expect(EN_solveH(p)).toBeLessThan(100);

    int index = 0;
    EN_getnodeindex(p, "J1", &index);
    double elevation = 0.0;
    EN_getnodevalue(p, index, EN_ELEVATION, &elevation);
    expect((int)elevation).toBe(150);

    EN_close(p);
    EN_deleteproject(p);
  });

  it("tears down a scripted model opened but never solved", [&]() {
    EN_Project p = nullptr;
    EN_createproject(&p);
    expect(EN_open(p, fixture("net-with-lua.inp").c_str(), kReportSink, ""))
        .toBe(0);
    expect(EN_getprivatedata(p)).toBeNotNull();
    EN_close(p);
    expect(EN_getprivatedata(p)).toBeNull();
    EN_deleteproject(p);
  });

  it("leaves a non-scripted model as stock EPANET", [&]() {
    EN_Project p = nullptr;
    EN_createproject(&p);
    expect(EN_open(p, fixture("net-no-lua.inp").c_str(), kReportSink, "")).toBe(0);
    expect(EN_getprivatedata(p)).toBeNull();

    std::string report;
    captureInto(p, &report);
    expect(EN_solveH(p)).toBeLessThan(100);
    EN_close(p);
    EN_deleteproject(p);

    expect(report.find("value is") == std::string::npos).toBe(true);
  });
});
