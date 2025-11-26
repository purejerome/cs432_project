#!/usr/bin/env bash
# Compare your P5 compiler to the reference compiler using
# inputs/, outputs/, and diffs/ under tests2/.
#
# Run this script from:  ~/cs432/cs432_project/p5-regalloc

set -u

# ---- paths you may need to tweak ----
REF="/cs/students/cs432/f25/decaf"   # reference compiler
ME="../decaf"                        # your compiler binary (relative to p5-regalloc)
BASE_DIR="$HOME/cs432/cs432_project/p5-regalloc"
REGS=8                               # number of physical registers (>= 2)
# ------------------------------------

cd "$BASE_DIR" || {
  echo "Could not cd to $BASE_DIR"
  exit 1
}

IN_DIR="tests2/inputs"
OUT_DIR="tests2/outputs"
DIFF_DIR="tests2/diffs"

mkdir -p "$IN_DIR" "$OUT_DIR" "$DIFF_DIR"

shopt -s nullglob
tests=( "$IN_DIR"/*.decaf )
shopt -u nullglob

if (( ${#tests[@]} == 0 )); then
  echo "No .decaf files found in $IN_DIR/"
  exit 0
fi

for src in "${tests[@]}"; do
  base="$(basename "$src" .decaf)"
  echo "=== $base ==="

  # ---------- output files ----------
  ref_run="$OUT_DIR/$base.ref.run"
  me_run="$OUT_DIR/$base.me.run"
  ref_alloc="$OUT_DIR/$base.ref.alloc"
  me_alloc="$OUT_DIR/$base.me.alloc"

  run_diff="$DIFF_DIR/$base.run.diff"
  alloc_diff="$DIFF_DIR/$base.alloc.diff"
  # ----------------------------------

  # ----- 1. Run both compilers normally (with execution) -----
  "$REF" -r "$REGS" "$src" > "$ref_run" 2>&1
  "$ME"  -r "$REGS" "$src" > "$me_run"  2>&1

  # ----- 2. Diff observable behavior -----
  if diff -u "$ref_run" "$me_run" > "$run_diff"; then
    echo "  RUN OK"
    rm -f "$run_diff"
  else
    echo "  RUN MISMATCH (see $run_diff)"
  fi

  # ----- 3. Dump post-regalloc ILOC (no execution) -----
  "$REF" -E -r "$REGS" --fdump-iloc-alloc "$src" > "$ref_alloc" 2>&1
  "$ME"  -E -r "$REGS" --fdump-iloc-alloc "$src" > "$me_alloc"  2>&1

  # ----- 4. Diff allocated ILOC -----
  if diff -u "$ref_alloc" "$me_alloc" > "$alloc_diff"; then
    echo "  ALLOC ILOC MATCH"
    rm -f "$alloc_diff"
  else
    echo "  ALLOC ILOC DIFFERENCES (see $alloc_diff; differences aren't automatically bugs)"
  fi
done
