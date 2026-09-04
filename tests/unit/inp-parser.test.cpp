#include <cest>

#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include <file-io.h>
#include <inp-parser.h>
}

static const char *fakeInp = nullptr;

static char *dupString(const char *s) {
  size_t n = std::strlen(s) + 1;
  char *out = (char *)std::malloc(n);
  std::memcpy(out, s, n);
  return out;
}

static char *stubReadFile(const char *path) {
  (void)path;
  return fakeInp ? dupString(fakeInp) : nullptr;
}

describe("InpParser", []() {
  beforeEach([]() { FileIO_SetReadFileFunction(stubReadFile); });
  afterEach([]() {
    FileIO_SetReadFileFunction(nullptr);
    fakeInp = nullptr;
  });

  it("extracts the [SCRIPT] body and stops at the next section", [&]() {
    fakeInp =
        "[JUNCTIONS]\n"
        " J1 100\n"
        "\n"
        "[SCRIPT]\n"
        "local t = {}\n"
        "t[1] = 42\n"
        "\n"
        "-- a comment\n"
        "local s = [[multi]]\n"
        "\n"
        "[COORDINATES]\n"
        " J1 0 0\n";
    char *script = InpParser_ReadScript("net.inp");
    expect(script).toBeNotNull();
    expect(std::string(script))
        .toEqual(
            "local t = {}\n"
            "t[1] = 42\n"
            "\n"
            "-- a comment\n"
            "local s = [[multi]]\n"
            "\n");
    std::free(script);
  });

  it("captures a [SCRIPT] section that runs to end of file", [&]() {
    fakeInp =
        "[SCRIPT]\n"
        "print('hi')\n"
        "return 1\n";
    char *script = InpParser_ReadScript("net.inp");
    expect(script).toBeNotNull();
    expect(std::string(script)).toEqual("print('hi')\nreturn 1\n");
    std::free(script);
  });

  it("returns NULL when there is no [SCRIPT] section", [&]() {
    fakeInp =
        "[JUNCTIONS]\n"
        " J1 100\n"
        "[END]\n";
    char *script = InpParser_ReadScript("net.inp");
    expect(script).toBeNull();
  });

  it("returns an empty string for a present but empty section", [&]() {
    fakeInp =
        "[SCRIPT]\n"
        "[END]\n";
    char *script = InpParser_ReadScript("net.inp");
    expect(script).toBeNotNull();
    expect(std::string(script)).toEqual("");
    std::free(script);
  });

  it("returns NULL when the file cannot be read", [&]() {
    fakeInp = nullptr;
    char *script = InpParser_ReadScript("missing.inp");
    expect(script).toBeNull();
  });

  it("does not end the section on Lua lines containing brackets", [&]() {
    fakeInp =
        "[SCRIPT]\n"
        "x = a[i]\n"
        "y = [[\n"
        "long\n"
        "]]\n"
        "[END]\n";
    char *script = InpParser_ReadScript("net.inp");
    expect(script).toBeNotNull();
    expect(std::string(script)).toEqual("x = a[i]\ny = [[\nlong\n]]\n");
    std::free(script);
  });
});
