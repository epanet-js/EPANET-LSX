#!/usr/bin/env bash
#
# init.sh — prepare everything the build and test steps need:
#
#   - the EPANET engine sources, pinned to a release tag  -> build/EPANET
#   - the Cest test header + cest-runner binary           -> build/cest
set -euo pipefail

EPANET_REPO="https://github.com/OpenWaterAnalytics/EPANET.git"
EPANET_TAG="v2.3.5"
CEST_RELEASE="https://github.com/cegonse/cest/releases/download/v5"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
EPANET_DIR="${BUILD_DIR}/EPANET"
CEST_DIR="${BUILD_DIR}/cest"

# ---------------------------------------------------------------------------
# Prerequisites: git, curl, cmake, and a working C and C++ compiler.
# ---------------------------------------------------------------------------
require() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "error: '$1' is required but was not found in PATH" >&2
    exit 1
  }
}
require git
require curl
require cmake

# Resolve a C compiler and prove it actually compiles/links a program.
if   [ -n "${CC:-}" ];                    then CC_BIN="$CC"
elif command -v cc  >/dev/null 2>&1;      then CC_BIN="cc"
elif command -v gcc >/dev/null 2>&1;      then CC_BIN="gcc"
elif command -v clang >/dev/null 2>&1;    then CC_BIN="clang"
else echo "error: a C compiler (cc/gcc/clang) is required" >&2; exit 1
fi

# Resolve a C++ compiler and prove it actually compiles/links a program.
if   [ -n "${CXX:-}" ];                   then CXX_BIN="$CXX"
elif command -v c++ >/dev/null 2>&1;      then CXX_BIN="c++"
elif command -v g++ >/dev/null 2>&1;      then CXX_BIN="g++"
elif command -v clang++ >/dev/null 2>&1;  then CXX_BIN="clang++"
else echo "error: a C++ compiler (c++/g++/clang++) is required" >&2; exit 1
fi

echo "==> checking C compiler ($CC_BIN)"
cc_check_dir="$(mktemp -d)"
printf 'int main(void){return 0;}\n' > "$cc_check_dir/c.c"
if ! "$CC_BIN" "$cc_check_dir/c.c" -o "$cc_check_dir/c.out" >/dev/null 2>&1; then
  rm -rf "$cc_check_dir"
  echo "error: the C compiler '$CC_BIN' failed to compile a trivial program." >&2
  exit 1
fi

echo "==> checking C++ compiler ($CXX_BIN)"
printf 'int main(){return 0;}\n' > "$cc_check_dir/cpp.cpp"
if ! "$CXX_BIN" "$cc_check_dir/cpp.cpp" -o "$cc_check_dir/cpp.out" >/dev/null 2>&1; then
  rm -rf "$cc_check_dir"
  echo "error: the C++ compiler '$CXX_BIN' failed to compile a trivial program." >&2
  exit 1
fi
rm -rf "$cc_check_dir"

if command -v sha256sum >/dev/null 2>&1; then
  sha256_of() { sha256sum "$1" | awk '{print $1}'; }
elif command -v shasum >/dev/null 2>&1; then
  sha256_of() { shasum -a 256 "$1" | awk '{print $1}'; }
else
  echo "error: need sha256sum or shasum to verify downloads" >&2
  exit 1
fi

mkdir -p "${BUILD_DIR}" "${CEST_DIR}"

# ---------------------------------------------------------------------------
# EPANET engine sources
# ---------------------------------------------------------------------------
if [ -d "${EPANET_DIR}" ]; then
  echo "==> EPANET sources already present at ${EPANET_DIR}"
else
  echo "==> cloning EPANET ${EPANET_TAG}"
  git clone --depth 1 --branch "${EPANET_TAG}" "${EPANET_REPO}" "${EPANET_DIR}"
fi

# ---------------------------------------------------------------------------
# cest-runner (per-platform prebuilt binary). If no binary matches this host,
# do not fail: scripts/run-tests.sh falls back to running the test_* binaries
# directly.
# ---------------------------------------------------------------------------
case "$(uname -s)" in
  Linux)  CEST_OS="linux" ;;
  Darwin) CEST_OS="macos" ;;
  *)      CEST_OS="windows" ;;
esac
case "$(uname -m)" in
  x86_64|amd64)  CEST_ARCH="x64" ;;
  aarch64|arm64) CEST_ARCH="aarch64" ;;
  i686|i386)     CEST_ARCH="x86" ;;
  *)             CEST_ARCH="" ;;
esac

CEST_ASSET=""
CEST_SHA=""
if [ -n "${CEST_ARCH}" ]; then
  CEST_ASSET="cest-runner-${CEST_OS}-${CEST_ARCH}"
  [ "${CEST_OS}" = "windows" ] && CEST_ASSET="${CEST_ASSET}.exe"
  case "${CEST_ASSET}" in
    cest-runner-linux-aarch64) CEST_SHA="260ac0ecf5a6223405a71dcb70cf35916c3fb8ffc1f9e8290cc788d254e23755" ;;
    cest-runner-linux-x64)     CEST_SHA="a227da96cfe59e6a29e8ab390cffdb507abe7ff24c8ace3e7ddd6caac38d956b" ;;
    cest-runner-linux-x86)     CEST_SHA="6026d144234a756ffdbd37f9000ed937282816a21e8a9fa224e20ef1edd436af" ;;
    cest-runner-macos-aarch64) CEST_SHA="0fca1326fd7382c3186e8719825cf04c7d020e46a57588d8ca80bf8d1cce53c6" ;;
    cest-runner-macos-x64)     CEST_SHA="90b2d1304036788ed01780c9022d11006ae861470345efb087990dc250816c18" ;;
    *)                         CEST_ASSET="" ;;  # no checksum listed -> treat as unavailable
  esac
fi

if [ -x "${CEST_DIR}/cest-runner" ]; then
  echo "==> cest-runner already present"
elif [ -z "${CEST_ASSET}" ]; then
  echo "  warning: no cest-runner binary for this platform ($(uname -s)/$(uname -m))." >&2
  echo "           tests will be run directly by scripts/run-tests.sh." >&2
else
  echo "==> downloading cest-runner (${CEST_ASSET})"
  if curl -fSL "${CEST_RELEASE}/${CEST_ASSET}" -o "${CEST_DIR}/cest-runner"; then
    actual_sha="$(sha256_of "${CEST_DIR}/cest-runner")"
    if [ "${actual_sha}" != "${CEST_SHA}" ]; then
      echo "  warning: cest-runner checksum mismatch; discarding it." >&2
      echo "    expected: ${CEST_SHA}" >&2
      echo "    actual:   ${actual_sha}" >&2
      rm -f "${CEST_DIR}/cest-runner"
    else
      chmod +x "${CEST_DIR}/cest-runner"
      "${CEST_DIR}/cest-runner" --help >/dev/null 2>&1 || {
        echo "  warning: cest-runner failed its smoke test; discarding it." >&2
        rm -f "${CEST_DIR}/cest-runner"
      }
    fi
  else
    echo "  warning: could not download cest-runner; tests will run directly." >&2
    rm -f "${CEST_DIR}/cest-runner"
  fi
fi

echo "init.sh: ready (EPANET in ${EPANET_DIR}, cest-runner in ${CEST_DIR})."
