#!/usr/bin/env bash

# ==============================================================================
# chingrep (cg) vs ripgrep Speed Arena
# Author: Wirote Chukeaw (chin6700x)
# Copyright: (c) 2026 Wirote Chukeaw. All rights reserved.
# License: MIT License
# ==============================================================================

DIR="$(cd "$(dirname "$0")/.." && pwd)"
TARGET_DIR="${1:-$DIR/..}"

# 1. Locate Ripgrep binary
RG_BIN="$(which rg 2>/dev/null)"
if [ -z "$RG_BIN" ] || [ ! -x "$RG_BIN" ]; then
  if [ -x "/Applications/ChatGPT.app/Contents/Resources/rg" ]; then
    RG_BIN="/Applications/ChatGPT.app/Contents/Resources/rg"
  elif [ -x "/Applications/Antigravity IDE.app/Contents/Resources/app/node_modules/@vscode/ripgrep/bin/rg" ]; then
    RG_BIN="/Applications/Antigravity IDE.app/Contents/Resources/app/node_modules/@vscode/ripgrep/bin/rg"
  fi
fi

# 2. Locate chingrep binary
CG_BIN="$DIR/chingrep"
if [ ! -x "$CG_BIN" ]; then
  make -C "$DIR" > /dev/null
fi

# 3. Locate nodegrep
NODE_GREP="$DIR/../node-grep/bin/nodegrep.js"

echo "=============================================================================="
echo "⚡ High-Performance Arena: chingrep (cg) vs ripgrep (Rust)"
echo "📂 Target Directory: $TARGET_DIR"
if [ -n "$RG_BIN" ]; then
  RG_VER=$("$RG_BIN" --version | head -n 1)
  echo "🦀 Ripgrep Engine:   $RG_VER ($RG_BIN)"
fi
echo "🌲 chingrep Engine:  $("$CG_BIN" --version)"
echo "=============================================================================="

# Helper function to measure execution duration in milliseconds
bench_cmd() {
  local cmd="$1"
  local start end diff
  start=$(python3 -c 'import time; print(int(time.time() * 1000))')
  eval "$cmd" > /dev/null 2>&1 || true
  end=$(python3 -c 'import time; print(int(time.time() * 1000))')
  diff=$((end - start))
  echo "$diff"
}

run_scenario() {
  local name="$1"
  local asm_cmd="$2"
  local rg_cmd="$3"

  echo ""
  echo "🔍 Scenario: $name"

  # Warm up disk caches
  eval "$asm_cmd" > /dev/null 2>&1 || true

  # Run 3 iterations and take minimum (best)
  local t_cg=999999
  local t_rg=999999

  for i in 1 2 3; do
    cur=$(bench_cmd "$cg_cmd")
    if [ "$cur" -lt "$t_cg" ]; then t_cg=$cur; fi
  done

  if [ -n "$RG_BIN" ]; then
    for i in 1 2 3; do
      cur=$(bench_cmd "$rg_cmd")
      if [ "$cur" -lt "$t_rg" ]; then t_rg=$cur; fi
    done
  fi

  printf "   🌲 chingrep (cg):    %5d ms" "$t_cg"
  if [ "$t_cg" -le "$t_rg" ]; then
    printf "  🥇 WINNER"
  fi
  echo ""

  if [ -n "$RG_BIN" ]; then
    printf "   🦀 ripgrep (Rust):   %5d ms" "$t_rg"
    if [ "$t_rg" -lt "$t_cg" ]; then
      printf "  🥇 WINNER"
    fi
    echo ""
  fi
}

run_scenario "1. Recursive Literal Search ('VirtualBuffer')" \
  "\"$CG_BIN\" -rn 'VirtualBuffer' \"$TARGET_DIR\"" \
  "\"$RG_BIN\" -n 'VirtualBuffer' \"$TARGET_DIR\""

run_scenario "2. Case-Insensitive Search ('tree-sitter')" \
  "\"$CG_BIN\" -rni 'tree-sitter' \"$TARGET_DIR\"" \
  "\"$RG_BIN\" -ni 'tree-sitter' \"$TARGET_DIR\""

run_scenario "3. Whole Word Search ('export')" \
  "\"$CG_BIN\" -rnw 'export' \"$TARGET_DIR\"" \
  "\"$RG_BIN\" -nw 'export' \"$TARGET_DIR\""

run_scenario "4. Count Matches ('ERROR')" \
  "\"$CG_BIN\" -rc 'ERROR' \"$TARGET_DIR\"" \
  "\"$RG_BIN\" -c 'ERROR' \"$TARGET_DIR\""

run_scenario "5. Files With Matches ('TreeCursor')" \
  "\"$CG_BIN\" -rl 'TreeCursor' \"$TARGET_DIR\"" \
  "\"$RG_BIN\" -l 'TreeCursor' \"$TARGET_DIR\""

echo ""
echo "=============================================================================="
echo "🏁 Benchmark Complete!"
echo "=============================================================================="
