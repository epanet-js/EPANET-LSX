#include <cest>

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <string>
#include <vector>

extern "C" {
#include <epanet2_2.h>
}

#ifndef FIXTURES_DIR
#define FIXTURES_DIR "."
#endif

static std::string net(const char *name) {
  return std::string(FIXTURES_DIR) + "/real-networks/" + name;
}

static void driveHydraulics(const char *file,
                            const std::function<void(EN_Project, long)> &onStep) {
  EN_Project p = nullptr;
  EN_createproject(&p);
  expect(EN_open(p, net(file).c_str(), "/dev/null", "")).toBeLessThan(100);
  expect(EN_openH(p)).toBeLessThan(100);
  expect(EN_initH(p, EN_NOSAVE)).toBeLessThan(100);

  long t = 0;
  long tstep = 0;
  do {
    expect(EN_runH(p, &t)).toBeLessThan(100);
    onStep(p, t);
    expect(EN_nextH(p, &tstep)).toBeLessThan(100);
  } while (tstep > 0);

  EN_closeH(p);
  EN_close(p);
  EN_deleteproject(p);
}

static int nodeIdx(EN_Project p, const char *id) {
  int i = 0;
  EN_getnodeindex(p, id, &i);
  return i;
}

static int linkIdx(EN_Project p, const char *id) {
  int i = 0;
  EN_getlinkindex(p, id, &i);
  return i;
}

static double pressure(EN_Project p, const char *id) {
  double v = 0.0;
  EN_getnodevalue(p, nodeIdx(p, id), EN_PRESSURE, &v);
  return v;
}

static double linkValue(EN_Project p, const char *id, int prop) {
  double v = 0.0;
  EN_getlinkvalue(p, linkIdx(p, id), prop, &v);
  return v;
}

static double tankLevel(EN_Project p, const char *id) {
  int i = nodeIdx(p, id);
  double head = 0.0;
  double elevation = 0.0;
  EN_getnodevalue(p, i, EN_HEAD, &head);
  EN_getnodevalue(p, i, EN_ELEVATION, &elevation);
  return head - elevation;
}

static double interpolate(double q, const std::vector<double> &qs,
                          const std::vector<double> &ps) {
  if (q <= qs.front()) return ps.front();
  if (q >= qs.back()) return ps.back();
  for (size_t i = 0; i + 1 < qs.size(); ++i) {
    if (q <= qs[i + 1]) {
      double f = (q - qs[i]) / (qs[i + 1] - qs[i]);
      return ps[i] + f * (ps[i + 1] - ps[i]);
    }
  }
  return ps.back();
}

static double vmin(const std::vector<double> &v) {
  return *std::min_element(v.begin(), v.end());
}
static double vmax(const std::vector<double> &v) {
  return *std::max_element(v.begin(), v.end());
}
static double vmean(const std::vector<double> &v) {
  return std::accumulate(v.begin(), v.end(), 0.0) / (double)v.size();
}
static double vmedian(std::vector<double> v) {
  std::sort(v.begin(), v.end());
  return v[v.size() / 2];
}

describe("remote-prv-setpoint: PRV holds 25 m at a remote control node", []() {
  it("keeps J126 at ~25 m across every timestep", [&]() {
    std::vector<double> pressures;
    driveHydraulics("remote-prv-setpoint.inp", [&](EN_Project p, long) {
      pressures.push_back(pressure(p, "J126"));
    });

    expect((int)pressures.size()).toBeGreaterThan(0);
    expect(vmin(pressures)).toBeGreaterThan(24.0);
    expect(vmax(pressures)).toBeLessThan(26.0);
  });
});

describe("float-valve: TCV throttles inflow so the tank never overflows", []() {
  it("holds the tank level in the regulation band below controlDepth (4.6 m)",
     [&]() {
       std::vector<double> level;
       driveHydraulics("float-valve.inp", [&](EN_Project p, long) {
        level.push_back(tankLevel(p, "T1"));
      });

       expect((int)level.size()).toBeGreaterThan(0);
       expect(vmax(level)).toBeLessThan(4.6);
       expect(vmin(level)).toBeGreaterThan(4.2);
     });
});

describe("flow-modulating-PRV: setpoint follows the flow lookup table", []() {
  it("holds J3 at the interpolated target for the measured flow", [&]() {
    std::vector<double> flows = {3.0, 6.0, 8.0, 10.0};
    std::vector<double> pressures = {20.0, 25.0, 29.0, 32.0};
    double worst = 0.0;
    driveHydraulics("flow-modulating-PRV.inp", [&](EN_Project p, long) {
      double target = interpolate(linkValue(p, "V1", EN_FLOW), flows, pressures);
      worst = std::max(worst, std::fabs(pressure(p, "J3") - target));
    });
    expect(worst).toBeLessThan(0.1);
  });
});

describe("flow-mod-prv-v4: setpoint follows the flow lookup table", []() {
  it("holds J138 at the interpolated target for the measured flow", [&]() {
    std::vector<double> flows = {3.0, 5.0, 7.0, 9.0};
    std::vector<double> pressures = {5.0, 6.0, 8.0, 10.0};
    double worst = 0.0;
    driveHydraulics("flow-mod-prv-v4.inp", [&](EN_Project p, long) {
      double target = interpolate(linkValue(p, "V2", EN_FLOW), flows, pressures);
      worst = std::max(worst, std::fabs(pressure(p, "J138") - target));
    });
    expect(worst).toBeLessThan(0.1);
  });
});

describe("PCV-as-PRV: PCV modulates percent open to hold J4 near 20 m", []() {
  it("keeps J4 within the +/- 0.2 m band around the 20 m set point", [&]() {
    std::vector<double> pressures;
    driveHydraulics("PCV-as-PRV.inp",
                    [&](EN_Project p, long) { pressures.push_back(pressure(p, "J4")); });

    expect(std::fabs(vmean(pressures) - 20.0)).toBeLessThan(0.2);
    expect(vmin(pressures)).toBeGreaterThan(19.75);
    expect(vmax(pressures)).toBeLessThan(20.25);
  });
});

describe("lua-vsp: variable-speed pump holds 25 m at the control node", []() {
  it("holds J126 at ~25 m for the large majority of timesteps", [&]() {
    std::vector<double> pressures;
    driveHydraulics("lua-vsp.inp",
                    [&](EN_Project p, long) { pressures.push_back(pressure(p, "J126")); });

    expect(std::fabs(vmedian(pressures) - 25.0)).toBeLessThan(0.2);
    int within = 0;
    for (double v : pressures) {
      if (std::fabs(v - 25.0) <= 1.0) within++;
    }
    expect(within * 100 / (int)pressures.size()).toBeGreaterThan(80);
  });
});

describe("Net3-time-delay-pumps: level-band station keeps tank 1 in band", []() {
  it("keeps tank 1 within the 12/16 band and cycles the duty pump", [&]() {
    std::vector<double> level;
    bool sawOn = false;
    bool sawOff = false;
    driveHydraulics("Net3-time-delay-pumps.inp", [&](EN_Project p, long) {
      level.push_back(tankLevel(p, "1"));
      double status = linkValue(p, "335", EN_STATUS);
      if (status >= 0.5) sawOn = true;
      else sawOff = true;
    });

    expect(vmin(level)).toBeGreaterThan(11.0);
    expect(vmax(level)).toBeLessThan(17.0);
    expect(sawOn).toBeTruthy();
    expect(sawOff).toBeTruthy();
  });
});

describe("time-based-level-control: pump band shifts with time of day", []() {
  it("fills tank T1 high overnight and lets it draw down during the day", [&]() {
    std::vector<double> nightLevel;
    std::vector<double> dayLevel;
    driveHydraulics("time-based-level-control.inp", [&](EN_Project p, long t) {
      double hour = (double)((t % 86400) / 3600);
      double level = tankLevel(p, "T1");
      if (hour >= 20.0 || hour < 7.0) nightLevel.push_back(level);
      else dayLevel.push_back(level);
    });

    expect((int)nightLevel.size()).toBeGreaterThan(0);
    expect((int)dayLevel.size()).toBeGreaterThan(0);
    expect(vmax(nightLevel)).toBeGreaterThan(7.0);
    expect(vmin(dayLevel)).toBeLessThan(6.6);
  });
});
