#!/usr/bin/env bash
set -euo pipefail

BIN="${DECaf_BIN:-./decaf}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
inputs="${root}/inputs"
expect="${root}/expect"
tmp_out="${root}/.out"
mkdir -p "${tmp_out}"

pass=0
fail=0

echo "[run] Running lexer tests..."
for f in "${inputs}"/*.decaf; do
  base="$(basename "${f}" .decaf)"
  exp_tokens="${expect}/${base}.tokens"
  exp_err="${expect}/${base}.errsubstr"
  out="${tmp_out}/${base}.tokens"
  err="${tmp_out}/${base}.stderr"

  if [[ -f "${exp_err}" ]]; then
    set +e
    "${BIN}" "${f}" >"${out}" 2>"${err}"
    rc=$?
    set -e
    if [[ $rc -eq 0 ]]; then
      echo "[-] ${base}: expected non-zero exit but got rc=0"
      ((fail++)) || true
      continue
    fi
    if grep -qiF "$(cat "${exp_err}")" "${err}"; then
      echo "[+] ${base}: error as expected"
      ((pass++)) || true
    else
      echo "[-] ${base}: error substring not found"
      echo "Expected substring:"
      cat "${exp_err}"
      echo "----- stderr -----"
      cat "${err}"
      ((fail++)) || true
    fi
  else
    "${BIN}" "${f}" >"${out}" 2>"${err}"
    if [[ -s "${err}" ]]; then
      echo "[-] ${base}: unexpected stderr output:"
      cat "${err}"
      ((fail++)) || true
      continue
    fi
    if diff -u "${exp_tokens}" "${out}"; then
      echo "[+] ${base}"
      ((pass++)) || true
    else
      echo "[-] ${base} (diff above)"
      ((fail++)) || true
    fi
  fi
done

echo
echo "Pass: ${pass}    Fail: ${fail}"
[[ ${fail} -eq 0 ]]
