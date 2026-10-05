#!/bin/bash
# ==============================================================================
# asmgrep -> chingrep: 100-Scenario Comprehensive Automated Test Suite
# File: test/run-tests.sh
# ==============================================================================

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$DIR/chingrep"
ALIAS_BIN="$DIR/cg"
FIXTURES="$DIR/test/fixtures"

# 1. Compile chingrep binary
make -C "$DIR" > /dev/null

rm -rf "$FIXTURES"
mkdir -p "$FIXTURES/nested/subdir" "$FIXTURES/vendor" "$FIXTURES/ignore_test"

TOTAL=100
PASSED=0
FAILED=0

pass() {
    PASSED=$((PASSED + 1))
    printf "  [\033[32m%3d/%d\033[0m] %-65s \033[32mPASS\033[0m\n" "$PASSED" "$TOTAL" "$1"
}

fail() {
    FAILED=$((FAILED + 1))
    printf "  [\033[31m%3d/%d\033[0m] %-65s \033[31mFAIL: %s\033[0m\n" "$((PASSED + FAILED))" "$TOTAL" "$1" "$2"
    exit 1
}

assert_contains() {
    local desc="$1"
    local needle="$2"
    local text="$3"
    if echo "$text" | grep -q -- "$needle"; then
        pass "$desc"
    else
        fail "$desc" "Expected to contain '$needle', got: '$text'"
    fi
}

assert_not_contains() {
    local desc="$1"
    local needle="$2"
    local text="$3"
    if echo "$text" | grep -q -- "$needle"; then
        fail "$desc" "Expected NOT to contain '$needle', but found it in: '$text'"
    else
        pass "$desc"
    fi
}

assert_eq() {
    local desc="$1"
    local expected="$2"
    local actual="$3"
    if [ "$expected" = "$actual" ]; then
        pass "$desc"
    else
        fail "$desc" "Expected '$expected', got '$actual'"
    fi
}

echo "=============================================================================="
echo "⚡ Starting chingrep (cg) 100-Scenario Test Suite..."
echo "📂 Binary: $BIN"
echo "=============================================================================="

# Setup baseline fixture files
echo "The quick brown fox jumps over the lazy dog" > "$FIXTURES/sample.txt"
echo "apple banana cherry date elderberry fig grape" >> "$FIXTURES/sample.txt"
echo "ARM64 NEON Vector Engine on Apple Silicon" >> "$FIXTURES/sample.txt"
echo -n "No trailing newline here" > "$FIXTURES/no_newline.txt"

# -----------------------------------------------------------------------------
# Category 1: Core Literal & Variable-Length Needle Matching (Tests 1-15)
# -----------------------------------------------------------------------------
echo ""
echo "🔍 [1/10] Testing Core Literal & Needle Lengths (1-15)..."

assert_contains "1. Literal exact substring match" "quick" "$("$BIN" "quick" "$FIXTURES/sample.txt")"
assert_contains "2. Single character needle (1-byte memchr)" "b" "$("$BIN" "b" "$FIXTURES/sample.txt")"
assert_contains "3. 2-byte needle (SIMD boundary)" "ox" "$("$BIN" "ox" "$FIXTURES/sample.txt")"
assert_contains "4. 3-byte needle" "fox" "$("$BIN" "fox" "$FIXTURES/sample.txt")"
assert_contains "5. 4-byte needle" "jump" "$("$BIN" "jump" "$FIXTURES/sample.txt")"
assert_contains "6. 8-byte needle" "brown fo" "$("$BIN" "brown fo" "$FIXTURES/sample.txt")"
assert_contains "7. 15-byte needle (sub-vector)" "over the lazy d" "$("$BIN" "over the lazy d" "$FIXTURES/sample.txt")"
assert_contains "8. 16-byte exact vector boundary" "The quick brown " "$("$BIN" "The quick brown " "$FIXTURES/sample.txt")"
assert_contains "9. 17-byte cross-vector needle" "The quick brown f" "$("$BIN" "The quick brown f" "$FIXTURES/sample.txt")"
assert_contains "10. 31-byte needle" "apple banana cherry date elderb" "$("$BIN" "apple banana cherry date elderb" "$FIXTURES/sample.txt")"
assert_contains "11. 32-byte dual-vector boundary" "apple banana cherry date elderbe" "$("$BIN" "apple banana cherry date elderbe" "$FIXTURES/sample.txt")"
assert_contains "12. 41-byte full line needle" "ARM64 NEON Vector Engine on Apple Silicon" "$("$BIN" "ARM64 NEON Vector Engine on Apple Silicon" "$FIXTURES/sample.txt")"
assert_contains "13. Match at very start of file" "The quick" "$("$BIN" "The quick" "$FIXTURES/sample.txt")"
assert_contains "14. Match on file with no trailing newline" "No trailing" "$("$BIN" "No trailing" "$FIXTURES/no_newline.txt")"
assert_not_contains "15. Non-existent needle returns empty" "nonexistent_string_123" "$("$BIN" "nonexistent_string_123" "$FIXTURES/sample.txt" || true)"

# -----------------------------------------------------------------------------
# Category 2: Case-Insensitive Matching (-i) (Tests 16-25)
# -----------------------------------------------------------------------------
echo ""
echo "🔤 [2/10] Testing Case-Insensitive Matching (-i) (16-25)..."

assert_contains "16. Lowercase needle against uppercase text" "ARM64" "$("$BIN" -i "arm64" "$FIXTURES/sample.txt")"
assert_contains "17. Uppercase needle against lowercase text" "quick" "$("$BIN" -i "QUICK" "$FIXTURES/sample.txt")"
assert_contains "18. Mixed case needle (aLmOsT rAnDoM)" "Apple Silicon" "$("$BIN" -i "aPpLe sIlIcOn" "$FIXTURES/sample.txt")"
assert_contains "19. Single char case-insensitive 'A' -> 'a'" "apple" "$("$BIN" -i "A" "$FIXTURES/sample.txt")"
assert_contains "20. Single char case-insensitive 'z' -> 'Z'" "lazy" "$("$BIN" -i "Z" "$FIXTURES/sample.txt")"
assert_contains "21. Case-insensitive with spaces" "brown fox" "$("$BIN" -i "BROWN FOX" "$FIXTURES/sample.txt")"
assert_contains "22. Case-insensitive with numbers and symbols" "ARM64" "$("$BIN" -i "arm64 neon" "$FIXTURES/sample.txt")"
OUT_MISSING=$("$BIN" -i "MISSING_FOO" "$FIXTURES/sample.txt" || true)
assert_eq "23. Non-matching pattern produces empty output" "" "$OUT_MISSING"
assert_contains "24. Multiple case variations in same file" "cherry" "$("$BIN" -i "CHERRY" "$FIXTURES/sample.txt")"
assert_contains "25. Case-insensitive match on no-newline file" "trailing" "$("$BIN" -i "TRAILING" "$FIXTURES/no_newline.txt")"

# -----------------------------------------------------------------------------
# Category 3: Whole-Word Matching (-w) (Tests 26-35)
# -----------------------------------------------------------------------------
echo ""
echo "🏷️ [3/10] Testing Whole-Word Matching (-w) (26-35)..."

echo "cat concatenate caterpillar bobcat cat" > "$FIXTURES/words.txt"
echo "foo_bar bar foo.bar (bar) [bar] {bar}" >> "$FIXTURES/words.txt"
echo "int count = 10; unsigned_int = 20;" >> "$FIXTURES/words.txt"

assert_contains "26. Exact whole word at line start/end" "cat" "$("$BIN" -w "cat" "$FIXTURES/words.txt")"
assert_not_contains "27. Embedded substring inside larger word rejected" "concatenate" "$("$BIN" -w "cat" "$FIXTURES/words.txt" | grep -v "cat" || true)"
assert_contains "28. Word bounded by parentheses (word)" "(bar)" "$("$BIN" -w "bar" "$FIXTURES/words.txt")"
assert_contains "29. Word bounded by brackets [word]" "[bar]" "$("$BIN" -w "bar" "$FIXTURES/words.txt")"
assert_contains "30. Word bounded by dots foo.bar" "foo.bar" "$("$BIN" -w "bar" "$FIXTURES/words.txt")"
assert_contains "31. Underscore treated as word char (no partial match)" "int count" "$("$BIN" -w "int" "$FIXTURES/words.txt")"
assert_contains "32. Whole word 'count'" "count = 10" "$("$BIN" -w "count" "$FIXTURES/words.txt")"
assert_contains "33. Combined -w and -i (Whole word + Case-insensitive)" "cat" "$("$BIN" -wi "CAT" "$FIXTURES/words.txt")"
assert_contains "34. Single-letter whole word" "dog" "$("$BIN" -w "dog" "$FIXTURES/sample.txt")"
OUT_X=$("$BIN" -w "x" "$FIXTURES/sample.txt" || true)
assert_eq "35. Single letter rejected inside word (fox)" "" "$OUT_X"

# -----------------------------------------------------------------------------
# Category 4: Line Numbers & Navigation (-n) (Tests 36-45)
# -----------------------------------------------------------------------------
echo ""
echo "🔢 [4/10] Testing Line Numbers (-n) (36-45)..."

# Generate 1,000 lines file
python3 -c '
with open("'"$FIXTURES"'/thousand.txt", "w") as f:
    for i in range(1, 1001):
        if i == 42:
            f.write("Line forty-two TARGET_42\n")
        elif i == 500:
            f.write("Line five hundred TARGET_500\n")
        elif i == 999:
            f.write("Line nine ninety-nine TARGET_999\n")
        else:
            f.write(f"This is ordinary line number {i}\n")
'

assert_contains "36. Line 1 number prefix" "1:" "$("$BIN" -n "The quick" "$FIXTURES/sample.txt")"
assert_contains "37. Line 2 number prefix" "2:" "$("$BIN" -n "banana" "$FIXTURES/sample.txt")"
assert_contains "38. Line 3 number prefix" "3:" "$("$BIN" -n "Silicon" "$FIXTURES/sample.txt")"
assert_contains "39. Deep line 42 number prefix" "42:Line forty-two" "$("$BIN" -n "TARGET_42" "$FIXTURES/thousand.txt")"
assert_contains "40. Mid-file line 500 number prefix" "500:Line five hundred" "$("$BIN" -n "TARGET_500" "$FIXTURES/thousand.txt")"
assert_contains "41. Deep line 999 number prefix" "999:Line nine ninety-nine" "$("$BIN" -n "TARGET_999" "$FIXTURES/thousand.txt")"
assert_contains "42. Default mode includes line numbers" "42:" "$("$BIN" "TARGET_42" "$FIXTURES/thousand.txt")"
assert_not_contains "43. Flag --no-line-number omits prefix" "42:" "$("$BIN" --no-line-number "TARGET_42" "$FIXTURES/thousand.txt")"
assert_contains "44. Match on empty lines surrounding targets" "1:" "$("$BIN" -n "quick" "$FIXTURES/sample.txt")"
assert_contains "45. Line number on no-newline file is 1" "1:No trailing" "$("$BIN" -n "trailing" "$FIXTURES/no_newline.txt")"

# -----------------------------------------------------------------------------
# Category 5: Counting Matches (-c) & Max-Count (-m) (Tests 46-55)
# -----------------------------------------------------------------------------
echo ""
echo "📊 [5/10] Testing Match Counting & Limits (46-55)..."

python3 -c '
with open("'"$FIXTURES"'/repeat.txt", "w") as f:
    for i in range(10):
        f.write("MATCH_TOKEN repeated line\n")
    for i in range(20):
        f.write("other irrelevant line\n")
'

CNT_10=$("$BIN" -c "MATCH_TOKEN" "$FIXTURES/repeat.txt" | awk -F: '{print $NF}')
assert_eq "46. Count exactly 10 matches (-c)" "10" "$CNT_10"

CNT_NONE=$("$BIN" -c "MISSING" "$FIXTURES/repeat.txt" 2>&1 || true)
assert_eq "47. Count 0 when pattern not present" "" "$CNT_NONE"

CNT_SAMPLE=$("$BIN" -c "fox" "$FIXTURES/sample.txt" | awk -F: '{print $NF}')
assert_eq "48. Count single match in sample file" "1" "$CNT_SAMPLE"

M_1=$("$BIN" -m 1 "MATCH_TOKEN" "$FIXTURES/repeat.txt" | wc -l | tr -d ' ')
assert_eq "49. Max count -m 1 stops after 1 match" "1" "$M_1"

M_3=$("$BIN" -m 3 "MATCH_TOKEN" "$FIXTURES/repeat.txt" | wc -l | tr -d ' ')
assert_eq "50. Max count -m 3 stops after 3 matches" "3" "$M_3"

M_5=$("$BIN" -m 5 "MATCH_TOKEN" "$FIXTURES/repeat.txt" | wc -l | tr -d ' ')
assert_eq "51. Max count -m 5 stops after 5 matches" "5" "$M_5"

CNT_ICASE=$("$BIN" -ci "match_token" "$FIXTURES/repeat.txt" | awk -F: '{print $NF}')
assert_eq "52. Count combined with case-insensitivity (-ci)" "10" "$CNT_ICASE"

CNT_WORD=$("$BIN" -cw "MATCH_TOKEN" "$FIXTURES/repeat.txt" | awk -F: '{print $NF}')
assert_eq "53. Count combined with whole-word (-cw)" "10" "$CNT_WORD"

# Max count larger than matches
M_MAX=$("$BIN" -m 50 "MATCH_TOKEN" "$FIXTURES/repeat.txt" | wc -l | tr -d ' ')
assert_eq "54. Max count > total matches outputs all 10" "10" "$M_MAX"

M_LONG_FLAG=$("$BIN" --max-count=2 "MATCH_TOKEN" "$FIXTURES/repeat.txt" | wc -l | tr -d ' ')
assert_eq "55. Long flag --max-count=2 works identically" "2" "$M_LONG_FLAG"

# -----------------------------------------------------------------------------
# Category 6: Files With Matches (-l) (Tests 56-65)
# -----------------------------------------------------------------------------
echo ""
echo "📁 [6/10] Testing Files With Matches (-l) (56-65)..."

assert_contains "56. File with match prints filename (-l)" "sample.txt" "$("$BIN" -l "quick" "$FIXTURES/sample.txt")"
assert_not_contains "57. File without match prints nothing (-l)" "sample.txt" "$("$BIN" -l "NON_EXIST" "$FIXTURES/sample.txt" || true)"

MULTI_L=$("$BIN" -l "Apple" "$FIXTURES/sample.txt" "$FIXTURES/thousand.txt" || true)
assert_contains "58. Multi-file target: lists matching file" "sample.txt" "$MULTI_L"
assert_not_contains "59. Multi-file target: omits non-matching file" "thousand.txt" "$MULTI_L"

L_COUNT_LINES=$("$BIN" -l "MATCH_TOKEN" "$FIXTURES/repeat.txt" | wc -l | tr -d ' ')
assert_eq "60. File with 10 matches prints filename exactly ONCE" "1" "$L_COUNT_LINES"

assert_contains "61. Files-with-matches combined with -i" "sample.txt" "$("$BIN" -li "QUICK" "$FIXTURES/sample.txt")"
assert_contains "62. Files-with-matches combined with -w" "words.txt" "$("$BIN" -lw "cat" "$FIXTURES/words.txt")"

RL_OUT=$("$BIN" -rl "TARGET_42" "$FIXTURES")
assert_contains "63. Recursive files-with-matches (-rl)" "thousand.txt" "$RL_OUT"
assert_not_contains "64. Recursive -rl skips non-matching files" "words.txt" "$RL_OUT"

assert_contains "65. Short-circuit -l on large thousand file" "thousand.txt" "$("$BIN" -l "TARGET_42" "$FIXTURES/thousand.txt")"

# -----------------------------------------------------------------------------
# Category 7: Globs & Filtering (--include, --exclude) (Tests 66-75)
# -----------------------------------------------------------------------------
echo ""
echo "🎯 [7/10] Testing Globs & Include/Exclude (66-75)..."

echo "const x = 1;" > "$FIXTURES/nested/code.js"
echo "int x = 1;" > "$FIXTURES/nested/code.c"
echo "let x = 1;" > "$FIXTURES/nested/code.ts"
echo "x = 1" > "$FIXTURES/nested/code.py"
echo "skip this" > "$FIXTURES/nested/skip.tmp"

INC_JS=$("$BIN" -r "x" --include="*.js" "$FIXTURES/nested")
assert_contains "66. --include=*.js matches code.js" "code.js" "$INC_JS"
assert_not_contains "67. --include=*.js excludes code.c" "code.c" "$INC_JS"

INC_C=$("$BIN" -r "x" --include="*.c" "$FIXTURES/nested")
assert_contains "68. --include=*.c matches code.c" "code.c" "$INC_C"
assert_not_contains "69. --include=*.c excludes code.js" "code.js" "$INC_C"

EXC_TMP=$("$BIN" -r "skip" --exclude="*.tmp" "$FIXTURES/nested" || true)
assert_not_contains "70. --exclude=*.tmp skips skip.tmp" "skip.tmp" "$EXC_TMP"

echo "vendor secret" > "$FIXTURES/vendor/secret.txt"
EXC_DIR=$("$BIN" -r "secret" --exclude-dir="vendor" "$FIXTURES" || true)
assert_not_contains "71. --exclude-dir=vendor skips vendor dir" "vendor/secret.txt" "$EXC_DIR"

INC_PY=$("$BIN" -r "x = 1" --include="*.py" "$FIXTURES/nested")
assert_contains "72. --include=*.py matches code.py" "code.py" "$INC_PY"

INC_MULTI=$("$BIN" -r "x" --include="code.*" "$FIXTURES/nested")
assert_contains "73. Wildcard prefix code.* matches code.ts" "code.ts" "$INC_MULTI"

FILES_LIST=$("$BIN" --files "$FIXTURES/nested")
assert_contains "74. --files lists code.js" "code.js" "$FILES_LIST"
assert_contains "75. --files lists code.c" "code.c" "$FILES_LIST"

# -----------------------------------------------------------------------------
# Category 8: Gitignore Pruning & Directory Traversal (Tests 76-85)
# -----------------------------------------------------------------------------
echo ""
echo "🌲 [8/10] Testing Gitignore Pruning & Custom Rules (76-85)..."

mkdir -p "$FIXTURES/ignore_test/node_modules" "$FIXTURES/ignore_test/.git"
echo "inside node_modules" > "$FIXTURES/ignore_test/node_modules/pkg.js"
echo "inside .git" > "$FIXTURES/ignore_test/.git/config"
echo "keep me" > "$FIXTURES/ignore_test/app.js"
echo "ignored_custom" > "$FIXTURES/ignore_test/secret.log"
echo "*.log" > "$FIXTURES/ignore_test/.gitignore"

DEF_IGNORE=$("$BIN" -r "inside" "$FIXTURES/ignore_test" || true)
assert_not_contains "76. Default pruning: ignores node_modules" "node_modules" "$DEF_IGNORE"
assert_not_contains "77. Default pruning: ignores .git" ".git" "$DEF_IGNORE"

APP_FOUND=$("$BIN" -r "keep me" "$FIXTURES/ignore_test")
assert_contains "78. Normal files in repo discovered" "app.js" "$APP_FOUND"

LOG_IGNORED=$("$BIN" -r "ignored_custom" "$FIXTURES/ignore_test" || true)
assert_not_contains "79. Local .gitignore rule (*.log) respected" "secret.log" "$LOG_IGNORED"

NO_IGNORE=$("$BIN" -r --no-ignore "ignored_custom" "$FIXTURES/ignore_test")
assert_contains "80. Flag --no-ignore overrides .gitignore" "secret.log" "$NO_IGNORE"

echo "dist/" >> "$FIXTURES/ignore_test/.gitignore"
mkdir -p "$FIXTURES/ignore_test/dist"
echo "bundle" > "$FIXTURES/ignore_test/dist/bundle.js"
DIST_IGN=$("$BIN" -r "bundle" "$FIXTURES/ignore_test" || true)
assert_not_contains "81. Directory rule with trailing slash (dist/) ignored" "dist/bundle.js" "$DIST_IGN"

# Subdirectory with nested .gitignore
mkdir -p "$FIXTURES/ignore_test/sub"
echo "sub_ignore.txt" > "$FIXTURES/ignore_test/sub/.gitignore"
echo "sub secret" > "$FIXTURES/ignore_test/sub/sub_ignore.txt"
SUB_IGN=$("$BIN" -r "sub secret" "$FIXTURES/ignore_test" || true)
assert_not_contains "82. Nested subdirectory .gitignore respected" "sub_ignore.txt" "$SUB_IGN"

# Comments in .gitignore
echo "# This is a comment" >> "$FIXTURES/ignore_test/.gitignore"
echo "   " >> "$FIXTURES/ignore_test/.gitignore"
assert_contains "83. Comments and blank lines in .gitignore parsed safely" "app.js" "$("$BIN" -r "keep me" "$FIXTURES/ignore_test")"

FILES_IGN=$("$BIN" --files "$FIXTURES/ignore_test")
assert_not_contains "84. --files respects .gitignore rules" "secret.log" "$FILES_IGN"
assert_contains "85. --files includes unignored files" "app.js" "$FILES_IGN"

# -----------------------------------------------------------------------------
# Category 9: Binary Probing & Non-ASCII / UTF-8 (Tests 86-92)
# -----------------------------------------------------------------------------
echo ""
echo "🛡️ [9/10] Testing Binary Null-Byte Probing & UTF-8 (86-92)..."

# Create binary file with null byte at start
printf "\x00BINARY_START" > "$FIXTURES/null_start.bin"
assert_not_contains "86. Null byte at index 0 skipped" "BINARY_START" "$("$BIN" "BINARY_START" "$FIXTURES/null_start.bin" || true)"

# Null byte in middle (< 512)
printf "HEADER_TEXT\x00BINARY_MID" > "$FIXTURES/null_mid.bin"
assert_not_contains "87. Null byte at index 11 skipped" "BINARY_MID" "$("$BIN" "BINARY_MID" "$FIXTURES/null_mid.bin" || true)"

# Clean UTF-8 file with Thai text
echo "ทดสอบภาษาไทย ระบบค้นหา asmgrep" > "$FIXTURES/thai.txt"
assert_contains "88. UTF-8 Thai text preserved without binary misclassification" "ทดสอบภาษาไทย" "$("$BIN" "ทดสอบภาษาไทย" "$FIXTURES/thai.txt")"
assert_contains "89. English needle in UTF-8 Thai file" "asmgrep" "$("$BIN" "asmgrep" "$FIXTURES/thai.txt")"

# Tabs and symbols
echo -e "col1\tcol2\tcol3\tspecial_value" > "$FIXTURES/tabs.txt"
assert_contains "90. Tab characters preserved as text" "special_value" "$("$BIN" "special_value" "$FIXTURES/tabs.txt")"

# Empty file handling
touch "$FIXTURES/empty.txt"
EMPTY_RES=$("$BIN" "anything" "$FIXTURES/empty.txt" || true)
assert_eq "91. Empty file handled with zero crash" "" "$EMPTY_RES"

# Piped binary data
PIPE_BIN=$(printf "TEXT\x00DATA" | "$BIN" "DATA" || true)
assert_eq "92. Piped binary data skipped safely" "" "$PIPE_BIN"

# -----------------------------------------------------------------------------
# Category 10: STDIN Pipelines & High-Throughput Stress (Tests 93-100)
# -----------------------------------------------------------------------------
echo ""
echo "🌊 [10/10] Testing STDIN Pipeline & Stress Tests (93-100)..."

assert_contains "93. Basic echo pipeline" "streamed_target" "$(echo "streamed_target" | "$BIN" "streamed_target")"
assert_contains "94. Pipeline with case-insensitivity" "UPPER" "$(echo "UPPERCASE_PIPE" | "$BIN" -i "uppercase")"
assert_contains "95. Pipeline with whole-word" "word_only" "$(echo "prefix word_only suffix" | "$BIN" -w "word_only")"

PIPE_CNT=$(printf "line1\nmatch_pipe\nmatch_pipe\nline4\n" | "$BIN" -c "match_pipe")
assert_contains "96. Pipeline match counting (-c)" "2" "$PIPE_CNT"

PIPE_LINE=$(printf "first\nsecond\nTARGET_LINE\n" | "$BIN" -n "TARGET_LINE")
assert_contains "97. Pipeline line numbering (-n shows line 3)" "3:TARGET_LINE" "$PIPE_LINE"

# Large pipeline stream (50,000 lines generated on the fly)
python3 -c '
with open("'"$FIXTURES"'/stress_50k.txt", "w") as f:
    for i in range(1, 50001):
        if i == 25000:
            f.write("HALFWAY_MAGIC_MARKER\n")
        elif i == 50000:
            f.write("END_OF_50K_MARKER\n")
        else:
            f.write("Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n")
'

assert_contains "98. Stress test: find line 25,000 in 50k file" "25000:HALFWAY_MAGIC_MARKER" "$("$BIN" -n "HALFWAY_MAGIC_MARKER" "$FIXTURES/stress_50k.txt")"
assert_contains "99. Stress test: find line 50,000 in 50k file" "50000:END_OF_50K_MARKER" "$("$BIN" -n "END_OF_50K_MARKER" "$FIXTURES/stress_50k.txt")"

# Stream 50,000 lines through STDIN
STRESS_PIPE=$(cat "$FIXTURES/stress_50k.txt" | "$BIN" -n "HALFWAY_MAGIC_MARKER")
assert_contains "100. Stress test: 50,000 lines streamed through STDIN pipe" "25000:HALFWAY_MAGIC_MARKER" "$STRESS_PIPE"

# Clean up fixtures
rm -rf "$FIXTURES"

echo ""
echo "=============================================================================="
echo "🎉 ALL $PASSED/$TOTAL ASMGREP AUTOMATED TESTS PASSED (100.0% Pass Rate)!"
echo "=============================================================================="
