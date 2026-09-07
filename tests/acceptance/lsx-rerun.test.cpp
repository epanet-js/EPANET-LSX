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

static int j1Elevation(EN_Project p) {
  int index = 0;
  EN_getnodeindex(p, "J1", &index);
  double value = 0.0;
  EN_getnodevalue(p, index, EN_ELEVATION, &value);
  return (int)value;
}

describe("LSX hydraulic re-run loop", []() {
  it("re-solves and re-runs the script up to MAX_LSX_ITERATIONS times", [&]() {
    EN_Project p = nullptr;
    EN_createproject(&p);
    expect(EN_open(p, fixture("net-lua-rerun-cap.inp").c_str(), LSX_NULL_DEVICE, ""))
        .toBe(0);
    expect(EN_solveH(p)).toBeLessThan(100);

    expect(j1Elevation(p)).toBe(100 + MAX_LSX_ITERATIONS);

    EN_close(p);
    EN_deleteproject(p);
  });

  it("stops re-running once the script reports no change", [&]() {
    EN_Project p = nullptr;
    EN_createproject(&p);
    expect(EN_open(p, fixture("net-lua-rerun-converge.inp").c_str(), LSX_NULL_DEVICE,
                   ""))
        .toBe(0);
    expect(EN_solveH(p)).toBeLessThan(100);

    expect(j1Elevation(p)).toBe(103);
    expect(103).toBeLessThan(100 + MAX_LSX_ITERATIONS);

    EN_close(p);
    EN_deleteproject(p);
  });
});
