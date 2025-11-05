#!/usr/bin/env bash
# Runs Decaf tests from inputs/, stores your outputs in outputs/, and diffs vs instructor in diffs/.
# Student compiler: ../decaf (override with ME_BIN)
# Instructor: /cs/students/cs432/f25/decaf (override with REF_BIN)
# Diff format: extra lines from *your* output (-); missing lines (+)

set -u

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Binaries (override via env if needed)
REF_BIN="${REF_BIN:-/cs/students/cs432/f25/decaf}"
ME_BIN="${ME_BIN:-${script_dir}/../decaf}"

INPUT_DIR="${script_dir}/inputs"
OUT_DIR="${script_dir}/outputs"
DIFF_DIR="${script_dir}/diffs"

mkdir -p "$OUT_DIR" "$DIFF_DIR"

# Optional flags via env (e.g., REF_FLAGS="--fdump-iloc" ME_FLAGS="--fdump-iloc")
# Safely split them into arrays:
read -r -a REF_OPTS <<< "${REF_FLAGS:-}"
read -r -a ME_OPTS  <<< "${ME_FLAGS:-}"

if [[ ! -x "$ME_BIN" ]]; then
  echo "ERROR: ${ME_BIN} not found or not executable." >&2
  exit 1
fi
if [[ ! -x "$REF_BIN" ]]; then
  echo "ERROR: Reference compiler not found at ${REF_BIN} (or not executable)." >&2
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
  # Strip CR and the simulator return line to avoid noise
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

  # Run reference and student compilers; normalize their outputs
  "$REF_BIN" "${REF_OPTS[@]}" "$t" 2>&1 | normalize > "$ref_out"
  "$ME_BIN"  "${ME_OPTS[@]}"  "$t" 2>&1 | normalize > "$my_out"

  # Create a diff that hides headers/context and flips signs so:
  #   extra lines from *your* output appear as '-' (extra),
  #   missing lines (present in ref, absent in yours) appear as '+'
  diff -u "$ref_out" "$my_out" \
    | awk '
        /^--- / || /^\+\+\+ / || /^@@/ { next }  # drop headers/hunks
        /^ / { next }                             # drop context
        /^-/ { print "+" substr($0,2); next }     # missing (you lack it)
        /^\+/ { print "-" substr($0,2); next }    # extra (you added it)
      ' > "$difffile"

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
