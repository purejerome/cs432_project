#!/usr/bin/env bash
# Runs Decaf tests from inputs/, stores your outputs in outputs/, and diffs vs instructor in diffs/.
# Student compiler: ../decaf
# Instructor: /cs/students/cs432/f25/decaf
# Diff format: extra lines from *your* output (-); missing lines (+)

set -u

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REF="/cs/students/cs432/f25/decaf"
ME="${script_dir}/../decaf"

INPUT_DIR="${script_dir}/inputs"
OUT_DIR="${script_dir}/outputs"
DIFF_DIR="${script_dir}/diffs"

mkdir -p "$OUT_DIR" "$DIFF_DIR"

if [[ ! -x "$ME" ]]; then
  echo "ERROR: ${ME} not found or not executable." >&2
  exit 1
fi
if [[ ! -x "$REF" ]]; then
  echo "ERROR: Reference compiler not found at $REF (or not executable)." >&2
  exit 1
fi

user_pat="${1:-*.decaf}"
shopt -s nullglob
mapfile -t tests < <(cd "$INPUT_DIR" && printf "%s\n" $user_pat)
shopt -u nullglob

if [[ ${#tests[@]} -eq 0 ]]; then
  echo "No tests found in $INPUT_DIR"
  exit 0
fi

normalize() {
  sed -e 's/\r$//' -e '/^RETURN VALUE = /d'
}

total=0
passed=0
failed=0

for rel in "${tests[@]}"; do
  t="${INPUT_DIR}/${rel}"
  base="${rel%.decaf}"
  my_out="${OUT_DIR}/${base}.out"
  ref_out="${OUT_DIR}/${base}.ref"
  difffile="${DIFF_DIR}/${base}.diff"

  ((total++))

  "$REF" "$t" 2>&1 | normalize > "$ref_out"
  "$ME"  "$t" 2>&1 | normalize > "$my_out"

  diff -u "$ref_out" "$my_out"         | sed -e '/^--- /d' -e '/^\+\+\+ /d' -e '/^@@/d' -e '/^ /d'               -e 's/^\(+.*\)$/\x01\1/' -e 's/^\(-.*\)$/\x02\1/'               -e 's/^\x01+/-/' -e 's/^\x02-/+/' > "$difffile"

  if [[ -s "$difffile" ]]; then
    echo "[FAIL] $rel  -->  diffs/${base}.diff"
    ((failed++))
  else
    echo "[PASS] $rel  -->  diffs/${base}.diff (empty)"
    ((passed++))
  fi
done

echo
echo "Summary: $passed passed, $failed failed, $total total."
[[ $failed -gt 0 ]] && exit 2 || exit 0
