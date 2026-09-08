// Drop-in verification for epanet2.dll on Windows.
//
// A host application built against stock EPANET resolves the engine's exports
// by their undecorated names (ENopen, not ENopen@4) at load time. This program
// reproduces exactly that: it LoadLibrary()s the DLL, resolves a handful of
// legacy-API exports by undecorated name via GetProcAddress(), and calls one to
// prove it is reachable through the correct calling convention. It deliberately
// does NOT link the DLL or its import library, so it tests the runtime export
// contract, not compile-time linkage.
//
// Usage: dll-smoke <path-to-epanet2.dll>   (defaults to "epanet2.dll")

#include <stdio.h>
#include <windows.h>

// The exports a legacy host application relies on. All must resolve undecorated.
static const char *kRequiredExports[] = {
    "ENgetversion", "ENepanet", "ENgeterror", "ENopen",
    "ENclose",      "ENsolveH", "ENgetnodevalue",
};

typedef int(__stdcall *ENgetversion_fn)(int *);

int main(int argc, char **argv) {
  const char *path = (argc > 1) ? argv[1] : "epanet2.dll";

  HMODULE dll = LoadLibraryA(path);
  if (dll == NULL) {
    fprintf(stderr, "FAIL: LoadLibrary(\"%s\") failed (error %lu)\n", path,
            (unsigned long)GetLastError());
    return 1;
  }
  printf("OK: loaded %s\n", path);

  int failed = 0;
  const int count = (int)(sizeof(kRequiredExports) / sizeof(kRequiredExports[0]));
  for (int i = 0; i < count; i++) {
    if (GetProcAddress(dll, kRequiredExports[i]) == NULL) {
      fprintf(stderr, "FAIL: export \"%s\" not found (decorated or missing)\n",
              kRequiredExports[i]);
      failed = 1;
    } else {
      printf("OK: resolved %s\n", kRequiredExports[i]);
    }
  }

  // Actually call one export to prove the export is reachable and the calling
  // convention matches (a mismatch would corrupt the stack / crash).
  ENgetversion_fn get_version =
      (ENgetversion_fn)GetProcAddress(dll, "ENgetversion");
  if (get_version != NULL) {
    int version = 0;
    int err = get_version(&version);
    printf("OK: ENgetversion() -> err=%d version=%d\n", err, version);
    if (err != 0 || version <= 0) {
      fprintf(stderr, "FAIL: ENgetversion returned err=%d version=%d\n", err,
              version);
      failed = 1;
    }
  } else {
    failed = 1;
  }

  FreeLibrary(dll);

  if (failed) {
    fprintf(stderr, "DLL smoke test FAILED\n");
    return 1;
  }
  printf("DLL smoke test PASSED\n");
  return 0;
}
