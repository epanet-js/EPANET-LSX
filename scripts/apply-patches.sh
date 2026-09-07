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

for patch in "${patches[@]}"; do
  echo "  -> Applying $(basename ${patch})..."
  git -C "${EPANET_DIR}" apply --whitespace=fix "${patch}"
  git -C "${EPANET_DIR}" add .
  git -C "${EPANET_DIR}" commit -m "LSX patch - ${patch}"
done
