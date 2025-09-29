#!/usr/bin/env bash
# Robust parser test runner for Decaf P2.
# Usage: ./run_parser_tests.sh /path/to/decaf

# --- Config ---------------------------------------------------------------

BIN="${1:-./decaf}"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set VERBOSE=1 to show program stderr/stdout for failing tests
VERBOSE="${VERBOSE:-0}"

should_parse=(
  01_minimal_main.decaf
  02_globals_and_arrays.decaf
  03_locals_assign_return.decaf
  04_if_else_while.decaf
  05_functions_and_calls.decaf
  06_expressions_precedence.decaf
  07_base_expressions.decaf
  08_func_calls_usage.decaf
  13_strings_and_escapes.decaf
  14_bool_assoc_shape.decaf
  15_return_forms.decaf
  16_unary_ops.decaf
  17_loop_break_continue.decaf
  19_empty_params_and_args.decaf
  20_expr_stress.decaf
)

should_fail=(
  09_err_missing_semicolon.decaf
  10_err_unmatched_brace.decaf
  11_err_bare_location_statement.decaf
  12_err_bad_token_in_expr.decaf
  18_err_call_missing_paren.decaf
)

# --- Helpers --------------------------------------------------------------

pass=0
fail=0

exists_or_die() {
  if [[ ! -x "$BIN" ]]; then
    echo "ERROR: Binary '$BIN' is not executable or not found."
    echo "Build your project first, e.g.: make clean && make"
    exit 2
  fi
}

run_case() {
  local file="$1"
  local expect="$2"  # "ok" or "fail"
  local path="$DIR/$file"

  if [[ ! -f "$path" ]]; then
    echo "[SKIP] $file (missing)"
    return
  fi

  # Run the program; capture output but keep exit code
  local out err rc
  out="$("$BIN" "$path" 2> >(err=$(cat); typeset -p err >/dev/null) )" || rc=$?
  rc=${rc:-0}

  if [[ "$expect" == "ok" ]]; then
    if [[ $rc -eq 0 ]]; then
      echo "[PASS] $file"
      ((pass++))
    else
      echo "[FAIL] $file (expected parse OK; exit $rc)"
      ((fail++))
      if [[ "$VERBOSE" == "1" ]]; then
        echo "----- STDERR -----"
        "$BIN" "$path" 2>&1 || true
        echo "------------------"
      fi
    fi
  else
    if [[ $rc -ne 0 ]]; then
      echo "[PASS] $file"
      ((pass++))
    else
      echo "[FAIL] $file (expected parse FAIL; exited 0)"
      ((fail++))
      if [[ "$VERBOSE" == "1" ]]; then
        echo "----- STDOUT -----"
        "$BIN" "$path" || true
        echo "------------------"
      fi
    fi
  fi
}

# --- Main -----------------------------------------------------------------

exists_or_die
echo "== Running parser tests with: $BIN =="

for f in "${should_parse[@]}"; do
  run_case "$f" ok
done

for f in "${should_fail[@]}"; do
  run_case "$f" fail
done

echo "== Done. Passed: $pass  Failed: $fail =="
# Exit nonzero if any failures
[[ $fail -gt 0 ]] && exit 1 || exit 0
