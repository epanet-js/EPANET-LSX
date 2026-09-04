#include <cest>

extern "C" {
#include <enext.h>
}

describe("ENEXT ABI", []() {
  it("pins the ABI version at 1", [&]() {
    expect((int)ENEXT_ABI_VERSION).toBe(1);
  });

  it("carries version and size fields in the host table", [&]() {
    ENEXT_HostApi host;
    host.api_version = ENEXT_ABI_VERSION;
    host.struct_size = (unsigned int)sizeof(ENEXT_HostApi);
    expect((int)host.api_version).toBe(1);
    expect(host.struct_size >= sizeof(unsigned int) * 2).toBe(true);
  });

  it("exposes the hand-written host services", [&]() {
    ENEXT_HostApi host;
    host.getprivatedata = nullptr;
    host.setprivatedata = nullptr;
    expect(host.getprivatedata == nullptr).toBe(true);
    expect(host.setprivatedata == nullptr).toBe(true);
  });

  it("expands the toolkit list into host-table members", [&]() {
    ENEXT_HostApi host;
    host.getcount = nullptr;
    host.getnodeindex = nullptr;
    host.getnodevalue = nullptr;
    host.getlinktype = nullptr;
    expect(host.getcount == nullptr).toBe(true);
    expect(host.getlinktype == nullptr).toBe(true);
  });
});
