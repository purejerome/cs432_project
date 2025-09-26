#!/usr/bin/env bash
# Compare your parser's AST output to the instructor's reference AST.
# Produces per-test folders with: input.decaf, output.*, expected.*, diff.txt
# Now automatically ignores lines like "RETURN VALUE = 0" in diffs.
# Usage:
#   ./compare_ast.sh <MY_DECAF_BIN> <REF_DECAF_BIN> [TEST_DIR=p2_parser_tests] [--filter PATTERN] [--verbose]

# set -e

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <MY_DECAF_BIN> <REF_DECAF_BIN> [TEST_DIR=p2_parser_tests] [--filter PATTERN] [--verbose]" >&2
  exit 2
fi

MY_BIN="$1"
REF_BIN="$2"
TEST_DIR="${3:-p2_parser_tests}"

FILTER=""
VERBOSE=0

# Parse optional flags/dir after first two args
shift 2
# If third param looked like a dir, keep it; otherwise default kept
if [[ $# -gt 0 && -d "$1" ]]; then
  TEST_DIR="$1"
  shift
fi
while [[ $# -gt 0 ]]; do
  case "$1" in
    --filter)
      shift
      FILTER="${1:-}"; [[ -z "$FILTER" ]] && { echo "ERROR: --filter needs a pattern"; exit 2; }
      ;;
    --filter=*)
      FILTER="${1#--filter=}"
      ;;
    --verbose)
      VERBOSE=1
      ;;
    *)
      echo "Ignoring unknown arg: $1" >&2
      ;;
  esac
  shift
done

# Checks
[[ -x "$MY_BIN" ]]  || { echo "ERROR: MY_BIN '$MY_BIN' not executable." >&2; exit 2; }
[[ -x "$REF_BIN" ]] || { echo "ERROR: REF_BIN '$REF_BIN' not executable." >&2; exit 2; }
[[ -d "$TEST_DIR" ]]|| { echo "ERROR: TEST_DIR '$TEST_DIR' not found." >&2; exit 2; }

mkdir -p results

# Collect tests
shopt -s nullglob
tests=( "$TEST_DIR"/*.decaf )
shopt -u nullglob
[[ ${#tests[@]} -gt 0 ]] || { echo "No .decaf files in '$TEST_DIR'." >&2; exit 1; }

echo "== Comparing ASTs =="
echo " My:  $MY_BIN"
echo " Ref: $REF_BIN"
echo " Dir: $TEST_DIR"
[[ -n "$FILTER" ]] && echo " Filter: $FILTER"
echo

pass=0
fail=0

for t in "${tests[@]}"; do
  base="$(basename "$t")"
  if [[ -n "$FILTER" && ! "$base" == $FILTER ]]; then
    continue
  fi

  case_dir="results/${base%.decaf}"
  rm -rf "$case_dir"
  mkdir -p "$case_dir"

  cp -f "$t" "$case_dir/input.decaf"

  # Run YOUR parser
  : > "$case_dir/output.txt"
  : > "$case_dir/output.err"
  if "$MY_BIN" "$t" >"$case_dir/output.txt" 2>"$case_dir/output.err"; then
    my_rc=0
  else
    my_rc=$?
  fi
  echo "$my_rc" > "$case_dir/output.rc"

  # Run REFERENCE parser (text AST)
  : > "$case_dir/expected.txt"
  : > "$case_dir/expected.err"
  if "$REF_BIN" --fdump-tree "$t" >"$case_dir/expected.txt" 2>"$case_dir/expected.err"; then
    ref_rc=0
  else
    ref_rc=$?
  fi
  echo "$ref_rc" > "$case_dir/expected.rc"

  # Filter out runtime prints like "RETURN VALUE = ..." from both before diff
  # (also strip trailing spaces for cleaner diffs)
  sed -E '/^RETURN VALUE = /d;s/[[:space:]]+$//' "$case_dir/expected.txt" > "$case_dir/expected.clean.txt"
  sed -E '/^RETURN VALUE = /d;s/[[:space:]]+$//' "$case_dir/output.txt"   > "$case_dir/output.clean.txt"

  diff_file="$case_dir/diff.txt"
  if [[ $ref_rc -eq 0 && $my_rc -eq 0 ]]; then
    if diff -u "$case_dir/expected.clean.txt" "$case_dir/output.clean.txt" >"$diff_file"; then
      echo "[PASS] $base"
      ((pass++))
      [[ $VERBOSE -eq 1 ]] && echo "(ASTs identical)"
    else
      echo "[FAIL] $base (AST mismatch)"
      ((fail++))
      [[ $VERBOSE -eq 1 ]] && { echo "----- DIFF -----"; head -200 "$diff_file"; echo "-----------------"; }
    fi
  else
    {
      echo "REF exit: $ref_rc"
      echo "MY  exit: $my_rc"
      echo
      echo "REF stderr:"
      sed -n '1,80p' "$case_dir/expected.err"
      echo
      echo "MY stderr:"
      sed -n '1,80p' "$case_dir/output.err"
    } > "$diff_file"

    if [[ $ref_rc -ne 0 && $my_rc -ne 0 ]]; then
      echo "[PASS] $base (both rejected input)"
      ((pass++))
    else
      echo "[FAIL] $base (exit-code mismatch: ref=$ref_rc, my=$my_rc)"
      ((fail++))
      [[ $VERBOSE -eq 1 ]] && { echo "----- SUMMARY -----"; cat "$diff_file"; echo "-------------------"; }
    fi
  fi
done

echo
echo "== Done. Passed: $pass  Failed: $fail =="
[[ $fail -gt 0 ]] && exit 1 || exit 0
