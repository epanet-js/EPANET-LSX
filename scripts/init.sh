#!/usr/bin/env bash
#
# init.sh — fetch the EPANET engine sources this extension builds on.
#
# Clones the EPANET repository at a pinned release tag into build/EPANET.
# Safe to re-run: if build/EPANET already exists it is left untouched.

set -euo pipefail

EPANET_REPO="https://github.com/OpenWaterAnalytics/EPANET.git"
EPANET_TAG="v2.3.5"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
DEST_DIR="${ROOT_DIR}/build/EPANET"

if [ -d "${DEST_DIR}" ]; then
  echo "EPANET sources already present at ${DEST_DIR}; nothing to do."
  echo "Remove it (or run 'make clean') to re-fetch."
  exit 0
fi

echo "Cloning EPANET ${EPANET_TAG} into ${DEST_DIR}..."
mkdir -p "${ROOT_DIR}/build"
git clone --depth 1 --branch "${EPANET_TAG}" "${EPANET_REPO}" "${DEST_DIR}"

echo "Done. EPANET ${EPANET_TAG} is available at ${DEST_DIR}."
