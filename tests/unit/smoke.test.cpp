#include <cest>

extern "C" {
#include <epanet2_2.h>
}

describe("EPANET-LSX smoke", []() {
  it("runs a trivial assertion", [&]() {
    expect(1 + 1).toBe(2);
  });

  it("links the EPANET toolkit", [&]() {
    int version = 0;
    EN_getversion(&version);
    expect(version).toBe(20305);
  });
});
