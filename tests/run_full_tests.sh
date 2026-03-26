#!/bin/bash
cd "$(dirname "$0")"
PLUGIN=../src/build/libChaosAndroid.so
SRC=test_basic.c
echo "=== COMPREHENSIVE TEST SUITE ==="

run() {
    rm -rf chaos-android-logs
    clang-17 -O1 -fpass-plugin=$PLUGIN $SRC -lm -o test_run 2>/dev/null
    result=$(./test_run 2>&1 | tail -1)
    echo "$1: $result"
}

printf "protections:\n  string_encryption: true\nlicense_key: CP-TEST-0000-0000-0000\nexclude:\n  - main\n" > chaos-android.yml
run "string_encryption"

printf "protections:\n  control_flow_flattening: true\nlicense_key: CP-TEST-0000-0000-0000\nexclude:\n  - main\n" > chaos-android.yml
run "CFF"

printf "protections:\n  opaque_constants: true\nlicense_key: CP-TEST-0000-0000-0000\nexclude:\n  - main\n" > chaos-android.yml
run "opaque_constants"

printf "protections:\n  arithmetic: true\nlicense_key: CP-TEST-0000-0000-0000\nexclude:\n  - main\n" > chaos-android.yml
run "arithmetic_MBA"

printf "protections:\n  indirect_branch: true\nlicense_key: CP-TEST-0000-0000-0000\nexclude:\n  - main\n" > chaos-android.yml
run "indirect_branch"

printf "protections:\n  control_flow_flattening: true\n  opaque_constants: true\n  arithmetic: true\nlicense_key: CP-TEST-0000-0000-0000\nexclude:\n  - main\n" > chaos-android.yml
run "CFF+opaque+arith"

printf "protections:\n  string_encryption: true\n  control_flow_flattening: true\n  opaque_constants: true\n  arithmetic: true\n  indirect_branch: true\nlicense_key: CP-TEST-0000-0000-0000\nexclude:\n  - main\n" > chaos-android.yml
run "ALL_obfuscation"

printf "protections:\n  ir_virtualization: true\nlicense_key: CP-TEST-0000-0000-0000\ninclude:\n  - add\n  - multiply\n  - fibonacci\n  - gcd\n  - popcount\n  - bit_reverse\n  - sort_array\n" > chaos-android.yml
run "VM_7_functions"

printf "protections:\n  control_flow_flattening: true\n  opaque_constants: true\n  ir_virtualization: true\nlicense_key: CP-TEST-0000-0000-0000\ninclude:\n  - add\n  - multiply\n  - fibonacci\n  - gcd\n" > chaos-android.yml
run "CFF+opaque+VM"

echo ""
echo "=== DONE ==="
