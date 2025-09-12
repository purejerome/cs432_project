#!/usr/bin/env bash
set -euo pipefail
BIN="${DECaf_BIN:-./decaf}"
INPUT="${1:-}"
if [[ -z "${INPUT}" ]]; then
  echo "Usage: $0 <path/to/file.decaf>" >&2
  exit 2
fi
"${BIN}" "${INPUT}"
