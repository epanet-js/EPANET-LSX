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
  name="$(basename "${patch}")"
  # Decide by a forward dry-run first: if the patch still applies it is not yet
  # applied, so apply it. Only when it does NOT apply forward do we treat a
  # clean reverse dry-run as "already applied". Testing reverse first gives a
  # false positive for deletion-only patches (reverse-adding lines only checks
  # surrounding context, not that the lines are absent), which would wrongly
  # skip them.
  if git -C "${EPANET_DIR}" apply --check "${patch}" >/dev/null 2>&1; then
    echo "  -> Applying ${name}..."
    git -C "${EPANET_DIR}" apply --whitespace=fix "${patch}"
    git -C "${EPANET_DIR}" add .
    git -C "${EPANET_DIR}" commit -m "LSX patch - ${patch}"
  elif git -C "${EPANET_DIR}" apply --reverse --check "${patch}" >/dev/null 2>&1; then
    echo "  -> ${name} already applied, skipping."
  else
    echo "ERROR: ${name} does not apply cleanly to ${EPANET_DIR}." >&2
    exit 1
  fi
done
