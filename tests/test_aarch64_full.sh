#!/bin/bash
set -e
PLUGIN=/mnt/c/Users/gb199/Desktop/antigravity_work/ChaosProtector-Android/src/build/libChaosAndroid.so
DIR=/mnt/c/Users/gb199/Desktop/antigravity_work/ChaosProtector-Android/tests
cd $DIR
SYSROOT=/root/android-sdk/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/sysroot
OBJDUMP=/root/android-sdk/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objdump

echo "=== AArch64 Android Cross-Compile Tests (system clang-17 + NDK sysroot) ==="
echo ""

run_test() {
    local name="$1"
    local yaml="$2"
    echo "$yaml" > chaos-android.yml
    rm -rf chaos-android-logs
    if clang-17 -O1 --target=aarch64-linux-android34 --sysroot=$SYSROOT -fPIC -shared -fpass-plugin=$PLUGIN test_minimal.c -o libtest.so 2>/dev/null; then
        echo "PASS: $name ($(wc -c < libtest.so) bytes)"
    else
        echo "FAIL: $name"
        return 1
    fi
}

# Test 1: Plain (no plugin) for baseline
clang-17 -O1 --target=aarch64-linux-android34 --sysroot=$SYSROOT -fPIC -shared test_minimal.c -o libtest_plain.so 2>/dev/null
echo "Baseline: $(wc -c < libtest_plain.so) bytes"
file libtest_plain.so
echo ""

# Test 2: String encryption
run_test "string_encryption" "protections:
  string_encryption: true
license_key: CP-TEST-0000-0000-0000"

# Test 3: Control Flow Flattening
run_test "control_flow_flattening" "protections:
  control_flow_flattening: true
license_key: CP-TEST-0000-0000-0000
exclude:
  - main"

# Test 4: Opaque Constants
run_test "opaque_constants" "protections:
  opaque_constants: true
license_key: CP-TEST-0000-0000-0000
exclude:
  - main"

# Test 5: Anti-Debug (AArch64 specific - should inject syscalls)
run_test "anti_debug" "protections:
  anti_debug: true
license_key: CP-TEST-0000-0000-0000"

# Verify syscalls were injected
svc_count=$($OBJDUMP -d libtest.so 2>/dev/null | grep -c 'svc' || echo 0)
chaos_funcs=$($OBJDUMP -t libtest.so 2>/dev/null | grep -c '__chaos_anti' || echo 0)
echo "  -> SVC instructions: $svc_count, __chaos_anti* symbols: $chaos_funcs"

# Test 6: Anti-Root
run_test "anti_root" "protections:
  anti_root: true
license_key: CP-TEST-0000-0000-0000"

root_strings=$(strings libtest.so 2>/dev/null | grep -c '/sbin/su\|magisk\|superuser' || echo 0)
echo "  -> Root check strings: $root_strings"

# Test 7: Anti-Tamper
run_test "anti_tamper" "protections:
  anti_tamper: true
license_key: CP-TEST-0000-0000-0000"

tamper_strings=$(strings libtest.so 2>/dev/null | grep -c 'frida\|xposed\|substrate' || echo 0)
echo "  -> Tamper check strings: $tamper_strings"

# Test 8: IR Virtualization
run_test "ir_virtualization" "protections:
  ir_virtualization: true
license_key: CP-TEST-0000-0000-0000
include:
  - add
  - fibonacci
  - is_prime
  - gcd"

bc_count=$($OBJDUMP -t libtest.so 2>/dev/null | grep -c '__chaos_bc' || echo 0)
vm_count=$($OBJDUMP -t libtest.so 2>/dev/null | grep -c '__chaos_vm_interpret' || echo 0)
echo "  -> Bytecode blobs: $bc_count, VM interpreter: $vm_count"

# Test 9: ALL passes combined
run_test "ALL_COMBINED" "protections:
  string_encryption: true
  control_flow_flattening: true
  opaque_constants: true
  indirect_branch: true
  anti_debug: true
  anti_root: true
  anti_tamper: true
  ir_virtualization: true
license_key: CP-TEST-0000-0000-0000
include:
  - add
  - fibonacci
exclude:
  - get_secret"

echo ""
echo "Plain:    $(wc -c < libtest_plain.so) bytes"
echo "Full:     $(wc -c < libtest.so) bytes"
echo "Ratio:    $(echo "scale=1; $(wc -c < libtest.so) * 100 / $(wc -c < libtest_plain.so)" | bc)%"

echo ""
echo "=== All AArch64 tests complete ==="
