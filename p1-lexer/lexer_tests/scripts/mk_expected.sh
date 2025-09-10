#!/usr/bin/env bash
set -euo pipefail
REF="${REF_DECAF_BIN:-/cs/students/cs432/f25/decaf}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
inputs="${root}/inputs"
expect="${root}/expect"
mkdir -p "${expect}"

echo "[regen] Using reference: ${REF}"
for f in "${inputs}"/*.decaf; do
  base="$(basename "${f}" .decaf)"
  if [[ -f "${expect}/${base}.errsubstr" ]]; then
    echo "[skip] ${base} is an error test"
    continue
  fi
  echo "[gen] ${base}"
  "${REF}" "${f}" > "${expect}/${base}.tokens"
done
