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
  # Use the commit log as the source of truth for "already applied". Each patch
  # is committed with a "LSX patch - <name>" marker, so a re-run skips patches
  # whose marker is already present. This is robust where a patch-vs-tree check
  # is not: once a later patch edits a region an earlier patch also touched
  # (e.g. 0004 rewrites EN_runH that 0003 introduced), the earlier patch no
  # longer applies forward or reverses cleanly, yet it is still applied.
  # Substring match in bash rather than `git log | grep -q`: with `pipefail`,
  # grep -q closing the pipe early makes git log exit with SIGPIPE, which would
  # mark the whole pipeline as failed even on a match and re-apply the patch.
  markers="$(git -C "${EPANET_DIR}" log --format=%s 2>/dev/null || true)"
  if [[ "${markers}" == *"LSX patch - ${name}"* ]]; then
    echo "  -> ${name} already applied, skipping."
    continue
  fi

  echo "  -> Applying ${name}..."
  if ! git -C "${EPANET_DIR}" apply "${patch}"; then
    echo "ERROR: ${name} does not apply cleanly to ${EPANET_DIR}." >&2
    exit 1
  fi
  git -C "${EPANET_DIR}" add .
  git -C "${EPANET_DIR}" commit -qm "LSX patch - ${name}"
done
