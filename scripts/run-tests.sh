#!/usr/bin/env bash
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
CEST_RUNNER="${BUILD_DIR}/cest/cest-runner"

if [ ! -d "${BUILD_DIR}" ]; then
  echo "error: ${BUILD_DIR} not found; build the tests first (make test)." >&2
  exit 1
fi

# Preferred path: let cest-runner discover and run the test executables.
if [ -x "${CEST_RUNNER}" ]; then
  echo "==> running tests with cest-runner"
  exec "${CEST_RUNNER}" "${BUILD_DIR}" "$@"
fi

# Fallback: no runner for this platform — run test_* binaries ourselves.
echo "==> cest-runner not available; running test_* binaries directly"

# Collect executables named test_* under build/ (sorted, NUL-safe).
tests=()
while IFS= read -r -d '' t; do
  tests+=("$t")
done < <(find "${BUILD_DIR}" -type f -name 'test_*' -perm -u+x -print0 2>/dev/null | sort -z)

if [ "${#tests[@]}" -eq 0 ]; then
  echo "no test_* binaries found under ${BUILD_DIR}; nothing to run."
  exit 0
fi

failures=0
for t in "${tests[@]}"; do
  echo "---- ${t#${ROOT_DIR}/} ----"
  if ! "$t"; then
    echo "FAILED: ${t#${ROOT_DIR}/}"
    failures=$((failures + 1))
  fi
done

echo
if [ "${failures}" -ne 0 ]; then
  echo "${failures} test binary(ies) failed."
  exit 1
fi
echo "all ${#tests[@]} test binary(ies) passed."
