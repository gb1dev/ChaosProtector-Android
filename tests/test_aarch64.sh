#!/bin/bash
SYSROOT=/root/android-sdk/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/sysroot
PLUGIN=/mnt/c/Users/gb199/Desktop/antigravity_work/ChaosProtector-Android/src/build/libChaosAndroid.so
SRC=/mnt/c/Users/gb199/Desktop/antigravity_work/ChaosProtector-Android/tests/test_minimal.c
OBJDUMP=/root/android-sdk/ndk/26.1.10909125/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-objdump
OUT=/tmp

echo "=== AArch64 Cross-Compile Tests ==="

# Plain baseline
clang-17 -O1 --target=aarch64-linux-android34 --sysroot=$SYSROOT -fPIC -shared $SRC -o $OUT/plain.so 2>&1
echo "Plain: $(wc -c < $OUT/plain.so) bytes ($(file $OUT/plain.so | grep -o 'ARM aarch64'))"

cd /mnt/c/Users/gb199/Desktop/antigravity_work/ChaosProtector-Android/tests

# Test each pass
for pass in string_encryption control_flow_flattening opaque_constants indirect_branch; do
    printf "protections:\n  $pass: true\nlicense_key: CP-TEST-0000-0000-0000\nexclude:\n  - main\n" > chaos-android.yml
    rm -rf chaos-android-logs
    if clang-17 -O1 --target=aarch64-linux-android34 --sysroot=$SYSROOT -fPIC -shared -fpass-plugin=$PLUGIN $SRC -o $OUT/test.so 2>/dev/null; then
        echo "PASS: $pass ($(wc -c < $OUT/test.so) bytes)"
    else
        echo "FAIL: $pass"
    fi
done

# Anti-debug (AArch64 specific!)
printf "protections:\n  anti_debug: true\nlicense_key: CP-TEST-0000-0000-0000\n" > chaos-android.yml
rm -rf chaos-android-logs
if clang-17 -O1 --target=aarch64-linux-android34 --sysroot=$SYSROOT -fPIC -shared -fpass-plugin=$PLUGIN $SRC -o $OUT/security.so 2>/dev/null; then
    svc=$($OBJDUMP -d $OUT/security.so 2>/dev/null | grep -c 'svc' || echo 0)
    echo "PASS: anti_debug ($svc SVC syscall instructions)"
else
    echo "FAIL: anti_debug"
fi

# Anti-root
printf "protections:\n  anti_root: true\nlicense_key: CP-TEST-0000-0000-0000\n" > chaos-android.yml
rm -rf chaos-android-logs
if clang-17 -O1 --target=aarch64-linux-android34 --sysroot=$SYSROOT -fPIC -shared -fpass-plugin=$PLUGIN $SRC -o $OUT/root.so 2>/dev/null; then
    paths=$(strings $OUT/root.so | grep -c '/sbin/su\|magisk\|superuser' || echo 0)
    echo "PASS: anti_root ($paths root path strings)"
else
    echo "FAIL: anti_root"
fi

# Anti-tamper
printf "protections:\n  anti_tamper: true\nlicense_key: CP-TEST-0000-0000-0000\n" > chaos-android.yml
rm -rf chaos-android-logs
if clang-17 -O1 --target=aarch64-linux-android34 --sysroot=$SYSROOT -fPIC -shared -fpass-plugin=$PLUGIN $SRC -o $OUT/tamper.so 2>/dev/null; then
    hooks=$(strings $OUT/tamper.so | grep -c 'frida\|xposed\|substrate' || echo 0)
    echo "PASS: anti_tamper ($hooks hook framework strings)"
else
    echo "FAIL: anti_tamper"
fi

# VM
printf "protections:\n  ir_virtualization: true\nlicense_key: CP-TEST-0000-0000-0000\ninclude:\n  - add\n  - fibonacci\n  - gcd\n" > chaos-android.yml
rm -rf chaos-android-logs
if clang-17 -O1 --target=aarch64-linux-android34 --sysroot=$SYSROOT -fPIC -shared -fpass-plugin=$PLUGIN $SRC -o $OUT/vm.so 2>/dev/null; then
    bc=$($OBJDUMP -t $OUT/vm.so 2>/dev/null | grep -c '__chaos_bc' || echo 0)
    echo "PASS: ir_virtualization ($bc bytecode blobs)"
else
    echo "FAIL: ir_virtualization"
fi

# ALL combined
printf "protections:\n  string_encryption: true\n  control_flow_flattening: true\n  opaque_constants: true\n  indirect_branch: true\n  anti_debug: true\n  anti_root: true\n  anti_tamper: true\n  ir_virtualization: true\nlicense_key: CP-TEST-0000-0000-0000\ninclude:\n  - add\n  - fibonacci\nexclude:\n  - get_secret\n" > chaos-android.yml
rm -rf chaos-android-logs
if clang-17 -O1 --target=aarch64-linux-android34 --sysroot=$SYSROOT -fPIC -shared -fpass-plugin=$PLUGIN $SRC -o $OUT/all.so 2>/dev/null; then
    echo "PASS: ALL_COMBINED ($(wc -c < $OUT/all.so) bytes vs $(wc -c < $OUT/plain.so) plain)"
else
    echo "FAIL: ALL_COMBINED"
fi

echo ""
echo "=== Done ==="
