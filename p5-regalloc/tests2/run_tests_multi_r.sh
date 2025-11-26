#!/usr/bin/env bash
# Run tests2/run_tests.sh for multiple register counts.

set -u

BASE_DIR="$HOME/cs432/cs432_project/p5-regalloc"
cd "$BASE_DIR" || {
  echo "Could not cd to $BASE_DIR"
  exit 1
}

for R in 1 2 3 4 5 6 7 8 16 99999999999; do
  echo "=============================="
  echo " Running tests with REGS=$R"
  echo "=============================="
  REGS="$R" tests2/run_tests.sh
  echo
done
