#!/usr/bin/env bash
#
# apply-patches.sh — apply every patch in patches/ to the EPANET sources.
#
# Patches are applied in sorted (numeric) order and each is idempotent:
# an already-applied patch is skipped, and a patch that does not apply
# cleanly aborts the script.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
EPANET_DIR="${ROOT_DIR}/build/EPANET"
PATCH_DIR="${ROOT_DIR}/patches"

if [ ! -d "${EPANET_DIR}" ]; then
  echo "EPANET sources missing at ${EPANET_DIR}; run 'make init' first." >&2
  exit 1
fi

shopt -s nullglob
patches=("${PATCH_DIR}"/*.patch)
shopt -u nullglob

if [ ${#patches[@]} -eq 0 ]; then
  echo "No patches found in ${PATCH_DIR}; nothing to apply."
  exit 0
fi

# Apply in sorted order (0001..., 0002..., ...).
IFS=$'\n' patches=($(sort <<<"${patches[*]}")); unset IFS

cd "${EPANET_DIR}"
for p in "${patches[@]}"; do
  name="$(basename "$p")"
  if git apply --check "$p" >/dev/null 2>&1; then
    git apply "$p" && echo "${name} applied."
  elif git apply --reverse --check "$p" >/dev/null 2>&1; then
    echo "${name} already applied."
  else
    echo "ERROR: ${name} does not apply cleanly to ${EPANET_DIR}." >&2
    exit 1
  fi
done
