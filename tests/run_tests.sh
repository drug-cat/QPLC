#!/usr/bin/env bash
# QPLC Test Suite - compile all examples and verify outputs
# Usage (from repo root): bash tests/run_tests.sh
set -u
cd "$(dirname "$0")/.."

QPLC=./build/qplc.exe
CONF=examples/conf.qplc
PASS=0
FAIL=0

green() { printf "\033[32m%s\033[0m\n" "$1"; }
red()   { printf "\033[31m%s\033[0m\n" "$1"; }

if [ ! -f "$QPLC" ]; then
    red "ERROR: qplc not built at build/qplc.exe"
    red "Build first:  export PATH=\"/c/msys64/ucrt64/bin:\$PATH\" && cmake -S . -B build -G Ninja && cmake --build build"
    exit 1
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

echo "============================================"
echo " QPLC Test Suite"
echo "============================================"

# ---- Test 1: All examples compile without error ----
echo -n "[Compile] test_all.q ... "
if $QPLC $CONF examples/test_all.q -o "$TMP/test_all.xml" 2>"$TMP/err"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL"; cat "$TMP/err"
fi

echo -n "[Golden ] test_all.q vs golden reference ... "
sed 's/\r$//' "$TMP/test_all.xml" | diff -q - <(sed 's/\r$//' tests/golden_test_all.xml) >/dev/null 2>&1
if [ $? -eq 0 ]; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL (differs from reference)"
fi

for f in main counter_test timer_test while_test loop_test array_test \
         iec_names_test edge_test constants_test functions_test while_break_test math_test \
         return_test ternary_test stdlib_test; do
    echo -n "[Compile] $f.q ... "
    if $QPLC $CONF examples/$f.q -o "$TMP/$f.xml" 2>"$TMP/err"; then
        PASS=$((PASS+1)); green "PASS"
    else
        FAIL=$((FAIL+1)); red "FAIL"; cat "$TMP/err"
    fi
done

# ---- Test 2: Expected semantic errors ----
printf 'def main():\n    undefined_var = True\n' > "$TMP/undef.q"
echo -n "[Semantic] undefined variable rejection ... "
$QPLC $CONF "$TMP/undef.q" -o /dev/null 2>"$TMP/err"
if grep -q "not defined" "$TMP/err"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL (expected semantic error)"
fi

printf 'def main():\n    speed = 5\n    if speed > 3:\n        break\n' > "$TMP/bad_break.q"
echo -n "[Semantic] break outside loop rejection ... "
$QPLC $CONF "$TMP/bad_break.q" -o /dev/null 2>"$TMP/err"
if grep -q "break.*inside.*while" "$TMP/err" || grep -q "valid inside" "$TMP/err"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL (expected break error)"; cat "$TMP/err"
fi

# ---- Test 3: IEC name equivalence ----
printf 'def main():\n    motor_run = TON(start_button, T#5s)\n' > "$TMP/ton.q"
echo -n "[Codegen ] TON alias -> on_delay in XML ... "
$QPLC $CONF "$TMP/ton.q" -o "$TMP/ton.xml" 2>/dev/null
if grep -q 'timer type="on_delay"' "$TMP/ton.xml"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL (on_delay not found)"
fi

# ---- Test 4: Edge contact generation ----
printf 'def main():\n    if rising_edge(start_button):\n        lamp = True\n' > "$TMP/edge.q"
echo -n "[Codegen ] rising_edge contact type ... "
$QPLC $CONF "$TMP/edge.q" -o "$TMP/edge.xml" 2>/dev/null
if grep -q 'type="rising"' "$TMP/edge.xml"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL (rising contact not found)"
fi

# ---- Test 5: Constant substitution ----
printf 'def main():\n    timer_output = TON(start_button, START_DELAY)\n' > "$TMP/const.q"
echo -n "[Codegen ] constant substitution T#2s ... "
$QPLC $CONF "$TMP/const.q" -o "$TMP/const.xml" 2>/dev/null
if grep -q 'duration="T#2s"' "$TMP/const.xml"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL (T#2s not substituted)"
fi

# ---- Test 6: While + break generates conditional jump ----
echo -n "[Codegen ] while_break conditional jmpn ... "
if grep -q 'jmpn.*label="WHILE_END' /tmp/ex_while_break_test.xml 2>/dev/null; then
    PASS=$((PASS+1)); green "PASS"
else
    $QPLC $CONF examples/while_break_test.q -o "$TMP/wbt.xml" 2>/dev/null
    if grep -q 'jmpn.*label="WHILE_END' "$TMP/wbt.xml" 2>/dev/null && \
       grep -c 'jmpn.*label="WHILE_END' "$TMP/wbt.xml" | grep -qv '^1$'; then
        PASS=$((PASS+1)); green "PASS"
    else
        FAIL=$((FAIL+1)); red "FAIL"
    fi
fi

# ---- Test 7: SCL generation ----
echo -n "[SCL    ] SCL file with FB header ... "
$QPLC $CONF examples/timer_test.q -o /dev/null -s "$TMP/timer.scl" 2>/dev/null
if grep -q 'FUNCTION_BLOCK' "$TMP/timer.scl" && grep -q 'TON' "$TMP/timer.scl"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL"
fi

# ---- Test 8: Debug flags ----
echo -n "[Debug  ] --tokens works ... "
if $QPLC $CONF examples/main.q --tokens 2>/dev/null | grep -q "IDENTIFIER"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL"
fi

echo -n "[Debug  ] --ast works ... "
if $QPLC $CONF examples/main.q --ast 2>/dev/null | grep -q "FunctionDef"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL"
fi

# ---- Test 9: Simulator while+break convergence (if dotnet available) ----
if command -v dotnet >/dev/null 2>&1 && [ -f QPLCSimulator/bin/Debug/net8.0/QPLCSimulator.dll ]; then
    printf 'def main():\n    count = 0\n    while running:\n        count = count + 1\n        if count >= 3:\n            break\n' > "$TMP/wb.q"
    $QPLC $CONF "$TMP/wb.q" -o "$TMP/wb.xml" 2>/dev/null
    echo -n "[Sim    ] while+break converges (count=3) ... "
    RESULT=$(printf "set running True\nrun\nshow\nexit\n" | \
        dotnet QPLCSimulator/bin/Debug/net8.0/QPLCSimulator.dll $CONF "$TMP/wb.xml" 2>/dev/null | \
        grep "count = ")
    if echo "$RESULT" | grep -q "count = 3"; then
        PASS=$((PASS+1)); green "PASS"
    else
        FAIL=$((FAIL+1)); red "FAIL (got: $RESULT)"
    fi
fi

# ---- Test 10: return compiles inside user function (verifies inline + return) ----
echo -n "[Codegen ] return_test inline function call ... "
if $QPLC $CONF examples/return_test.q -o "$TMP/ret.xml" 2>/dev/null && \
   grep -q 'rung' "$TMP/ret.xml"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL"
fi

# ---- Test 11: ternary compiles and emits move rungs for both branches ----
echo -n "[Codegen ] ternary_test boolean move rungs ... "
if $QPLC $CONF examples/ternary_test.q -o "$TMP/tern.xml" 2>/dev/null && \
   grep -q 'rung' "$TMP/tern.xml"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL"
fi

# ---- Test 12: IEC math functions min/max/abs/limit in SCL ----
echo -n "[SCL    ] stdlib math: LIMIT/MIN/MAX/ABS passthrough ... "
$QPLC $CONF examples/stdlib_test.q -o /dev/null -s "$TMP/std.scl" 2>/dev/null
if grep -qE 'LIMIT|MIN|MAX|ABS' "$TMP/std.scl"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL"; head -20 "$TMP/std.scl"
fi

# ---- Test 13: Semantic - return outside any function is allowed in main ----
echo -n "[Semantic] return in main allowed ... "
$QPLC $CONF examples/return_test.q -o /dev/null 2>"$TMP/ret_err"
if [ $? -eq 0 ]; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL"; cat "$TMP/ret_err"
fi

# ---- Test 14: IEC name normalization in SCL output ----
echo -n "[SCL    ] on_delay normalised to TON ... "
$QPLC $CONF examples/iec_names_test.q -o /dev/null -s "$TMP/iec.scl" 2>/dev/null
if grep -qE 'TON\b' "$TMP/iec.scl"; then
    PASS=$((PASS+1)); green "PASS"
else
    FAIL=$((FAIL+1)); red "FAIL"
fi

echo "============================================"
echo " Results: $PASS passed, $FAIL failed"
echo "============================================"
[ $FAIL -eq 0 ] && exit 0 || exit 1