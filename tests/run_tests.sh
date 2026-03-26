#!/bin/bash
cd "$(dirname "$0")"

PLUGIN=../src/build/libChaosAndroid.so
SRC=test_basic.c

echo "=== ChaosProtector Android Test Suite ==="

run_test() {
    local name=$1
    shift
    cat > chaos-android.yml <<EOF
protections:
$@
license_key: CP-TEST-0000-0000-0000
exclude:
  - main
EOF
    rm -rf chaos-android-logs
    clang-17 -O1 -fpass-plugin=$PLUGIN $SRC -lm -o test_fn 2>/dev/null
    if ./test_fn >/dev/null 2>&1; then
        echo "PASS: $name"
    else
        result=$(./test_fn 2>&1 | grep -c FAIL)
        echo "FAIL: $name ($result failures)"
    fi
}

run_test "string_encryption" "  string_encryption: true"
run_test "control_flow_flattening" "  control_flow_flattening: true"
run_test "opaque_constants" "  opaque_constants: true"
run_test "arithmetic_SKIP_x86" "  arithmetic: true"  # Known x86_64 issue - MBA designed for AArch64
run_test "indirect_branch" "  indirect_branch: true"
run_test "cff+opaque+arith" "  control_flow_flattening: true
  opaque_constants: true
  arithmetic: true"
run_test "all_obf_no_vm" "  string_encryption: true
  control_flow_flattening: true
  opaque_constants: true
  arithmetic: true
  indirect_branch: true"

# VM tests
cat > chaos-android.yml <<EOF
protections:
  ir_virtualization: true
license_key: CP-TEST-0000-0000-0000
include:
  - add
  - multiply
  - fibonacci
  - gcd
  - popcount
  - bit_reverse
  - sort_array
EOF
rm -rf chaos-android-logs
clang-17 -O1 -fpass-plugin=$PLUGIN $SRC -lm -o test_fn 2>/dev/null
if ./test_fn >/dev/null 2>&1; then
    echo "PASS: vm_all_functions"
else
    result=$(./test_fn 2>&1 | grep -c FAIL)
    echo "FAIL: vm_all_functions ($result failures)"
fi

echo "=== Done ==="
