#!/bin/bash
set -e

NDK=/root/android-sdk/ndk/26.1.10909125
CLANG=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android34-clang
PLUGIN=/mnt/c/Users/gb199/Desktop/antigravity_work/ChaosProtector-Android/src/build/libChaosAndroid.so
DIR=/mnt/c/Users/gb199/Desktop/antigravity_work/ChaosProtector-Android/tests
cd $DIR

echo "=== NDK AArch64 Cross-Compile Tests ==="
echo "NDK Clang: $($CLANG --version 2>&1 | head -1)"
echo ""

# Test 1: Plain compile (no plugin)
echo "--- Test 1: Plain NDK compile ---"
$CLANG -O1 -fPIC -shared test_minimal.c -o libtest_plain.so 2>&1
file libtest_plain.so
echo ""

# Test 2: With CFF + Opaque + IndirectBranch
echo "--- Test 2: CFF + Opaque + IndirectBranch ---"
cat > chaos-android.yml << 'EOF'
protections:
  control_flow_flattening: true
  opaque_constants: true
  indirect_branch: true
license_key: CP-TEST-0000-0000-0000
EOF
rm -rf chaos-android-logs
$CLANG -O1 -fPIC -shared -fpass-plugin=$PLUGIN test_minimal.c -o libtest_obf.so 2>&1
if [ $? -eq 0 ]; then
    echo "PASS: Obfuscation compile"
    file libtest_obf.so
    echo "Plain: $(wc -c < libtest_plain.so) bytes"
    echo "Obfuscated: $(wc -c < libtest_obf.so) bytes"
else
    echo "FAIL: Obfuscation compile"
fi
echo ""

# Test 3: Anti-Debug + Anti-Root + Anti-Tamper (AArch64 specific)
echo "--- Test 3: Anti-Debug + Anti-Root + Anti-Tamper ---"
cat > chaos-android.yml << 'EOF'
protections:
  anti_debug: true
  anti_root: true
  anti_tamper: true
license_key: CP-TEST-0000-0000-0000
EOF
rm -rf chaos-android-logs
$CLANG -O1 -fPIC -shared -fpass-plugin=$PLUGIN test_minimal.c -o libtest_security.so 2>&1
if [ $? -eq 0 ]; then
    echo "PASS: Security passes compile"
    file libtest_security.so
    # Verify syscall instructions are present
    echo "SVC instructions (syscalls): $($NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objdump -d libtest_security.so 2>/dev/null | grep -c 'svc')"
    echo "Anti-debug/root/tamper constructors: $($NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objdump -t libtest_security.so 2>/dev/null | grep -c '__chaos_anti')"
else
    echo "FAIL: Security passes compile"
fi
echo ""
cat chaos-android-logs/chaos-android-init.log 2>/dev/null
echo ""
cat chaos-android-logs/chaos-android-module-logs/aarch64/*.log 2>/dev/null | grep -E "Changes|injected" | head -10
echo ""

# Test 4: IR Virtualization on AArch64
echo "--- Test 4: IR Virtualization ---"
cat > chaos-android.yml << 'EOF'
protections:
  ir_virtualization: true
license_key: CP-TEST-0000-0000-0000
include:
  - add
  - fibonacci
  - is_prime
  - gcd
EOF
rm -rf chaos-android-logs
$CLANG -O1 -fPIC -shared -fpass-plugin=$PLUGIN test_minimal.c -o libtest_vm.so 2>&1
if [ $? -eq 0 ]; then
    echo "PASS: VM compile"
    file libtest_vm.so
    echo "Bytecode symbols: $($NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objdump -t libtest_vm.so 2>/dev/null | grep -c '__chaos_bc')"
    echo "VM interpreter: $($NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objdump -t libtest_vm.so 2>/dev/null | grep -c '__chaos_vm_interpret')"
else
    echo "FAIL: VM compile"
fi
echo ""
cat chaos-android-logs/chaos-android-module-logs/aarch64/*.log 2>/dev/null | grep -E "Compiled|Virtual" | head -10
echo ""

# Test 5: ALL passes combined
echo "--- Test 5: ALL passes combined ---"
cat > chaos-android.yml << 'EOF'
protections:
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
  - main
  - get_secret
EOF
rm -rf chaos-android-logs
$CLANG -O1 -fPIC -shared -fpass-plugin=$PLUGIN test_minimal.c -o libtest_all.so 2>&1
if [ $? -eq 0 ]; then
    echo "PASS: All passes compile"
    echo "Plain: $(wc -c < libtest_plain.so) bytes"
    echo "All protections: $(wc -c < libtest_all.so) bytes"
    echo "Ratio: $(echo "scale=1; $(wc -c < libtest_all.so) / $(wc -c < libtest_plain.so)" | bc)x"
else
    echo "FAIL: All passes compile"
fi

echo ""
echo "=== All NDK tests complete ==="
