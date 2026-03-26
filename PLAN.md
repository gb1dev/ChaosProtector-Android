# ChaosProtector Android — Development Plan

Forked from [O-MVLL](https://github.com/open-obfuscator/o-mvll) (Apache 2.0).
LLVM-based obfuscation for Android native code (.so) and iOS.

## Architecture

```
Developer's C/C++ source code
        ↓
    NDK Clang compiler
        ↓
    LLVM IR (Intermediate Representation)
        ↓
    ChaosProtector Android plugin (-fpass-plugin=libChaosAndroid.so)
        ↓  Applies obfuscation passes on IR
    LLVM ARM64/ARM backend
        ↓
    Protected .so library
        ↓
    APK/AAB package
```

The plugin runs on the developer's machine (Windows x64 / macOS / Linux x64).
It manipulates LLVM IR, not raw ARM64 instructions. LLVM handles all architecture-specific encoding.

## What O-MVLL Already Has (16 passes, ~7800 LOC)

| Pass | LOC | Status |
|------|-----|--------|
| String Encryption | 756 | Working (5 encoding strategies) |
| Control Flow Flattening | 500 | Working (state machine dispatch) |
| Opaque Constants | 358 | Working |
| MBA Arithmetic | 353 | Working |
| Opaque Field Access | 386 | Working |
| Function Outlining | 241 | Working |
| Break Control Flow | 273 | Working |
| Indirect Branches | 173 | Working |
| Basic Block Duplication | 201 | Working |
| Indirect Calls | 133 | Working |
| Anti-Hook (Frida) | 100 | Working |
| ObjCleaner (iOS) | 83 | Working |
| Cleaning | 54 | Working |
| Metadata | - | Working |
| Logger | 26 | Working |

## What We Add

### Phase 1: Rebrand + Licensing + Simplify Config
- [x] Rename from O-MVLL to ChaosProtector Android
- [x] Add ChaosProtector licensing (HWID + RSA verification)
- [x] Tier gating: Free (string enc + anti-hook), Basic (+ CFF + opaque + arith), Pro (all)
- [x] Simplified YAML config (no Python required for basic usage)
- [x] Gradle plugin for one-line integration
- [x] Build on Linux x64 (tested with LLVM 17, 17/17 tests pass, 51x IR complexity increase)
- [ ] Build for Windows x64 + macOS ARM64

### Phase 2: New Passes
- [x] Enhanced Anti-Debug: ptrace syscall + /proc/self/status TracerPid check
- [x] Anti-Root: 20 root paths (su, Magisk, SuperSU) + SELinux permissive check
- [x] Anti-Tamper: /proc/self/maps scan for Frida/Xposed/Substrate injection
- [ ] Resource Encryption: encrypt assets in APK, decrypt at runtime

### Phase 3: IR Virtualization (Killer Feature)
- [x] VM opcode set designed (60+ opcodes: stack ops, arithmetic, bitwise, comparisons, memory, control flow, calls, type conversions, floating point)
- [x] IR-to-bytecode compiler (converts LLVM IR to VM bytecode, supports all common instruction types)
- [x] VM interpreter emitted as LLVM IR (embedded in target .so, ~40 basic blocks)
- [x] Per-function virtualization via YAML include/exclude lists
- [x] Tested: add, multiply, fibonacci, gcd, sort_array, bit_reverse, popcount all pass individually
- [ ] Fix multi-function virtualization (stack/memory issue when >2 functions virtualized simultaneously)
- [ ] Opcode randomization (polymorphic VM - different opcodes per build)
- [ ] This is the feature NO other LLVM-based obfuscator has

### Phase 4: DEX Protection
- [x] String encryption in DEX (XOR + position cipher + Base64, ASM library)
- [x] Control flow flattening on DEX bytecode (state-machine dispatcher)
- [x] DexProtector module with CLI + library API
- [x] Unified Gradle plugin: native + DEX protections in one DSL
- [ ] Full integration testing with real Android APK

## Build Targets

| Platform | Architecture | LLVM | NDK |
|----------|-------------|------|-----|
| Android | ARM64 (AArch64) | 17 | r26d |
| Android | ARM (ARMv7) | 17 | r26d |
| iOS | ARM64 | 19.1.4 | Xcode 16 |

Host platforms (where the tool runs):
- Windows x64
- Linux x64
- macOS ARM64 (Apple Silicon)

## Configuration (Simplified)

```yaml
# chaos-android.yml (project root)
protections:
  string_encryption: true
  control_flow_flattening: true
  opaque_constants: true
  anti_hook: true
  anti_debug: true

# Per-function overrides
exclude:
  - "JNI_OnLoad"
  - "Java_*"  # Skip JNI bridge functions

include_virtualization:
  - "verify_license"
  - "decrypt_payload"
  - "check_integrity"
```

## Gradle Integration

```groovy
// build.gradle
plugins {
    id 'com.chaosprotector.android' version '1.0.0'
}

chaosProtector {
    licenseKey = "CP-XXXX-XXXX-XXXX-XXXX"

    protections {
        stringEncryption = true
        controlFlowFlattening = true
        antiDebug = true
    }
}
```

## Pricing (Same Tiers as ChaosProtector)

| Feature | Free | Basic ($99) | Pro ($199) |
|---------|------|-------------|------------|
| String Encryption | Yes | Yes | Yes |
| Anti-Hook (Frida) | Yes | Yes | Yes |
| Control Flow Flattening | No | Yes | Yes |
| Opaque Constants | No | Yes | Yes |
| MBA Arithmetic | No | Yes | Yes |
| Indirect Branches/Calls | No | Yes | Yes |
| Anti-Debug | No | Yes | Yes |
| Anti-Tamper | No | Yes | Yes |
| IR Virtualization | No | No | Yes |
| DEX Protection | No | No | Yes |

## Repository Structure

```
ChaosProtector-Android/
├── src/
│   ├── core/           ← Plugin infrastructure (from O-MVLL)
│   ├── passes/         ← Obfuscation passes (from O-MVLL + new)
│   │   ├── string-encoding/
│   │   ├── cfg-flattening/
│   │   ├── anti-debug/      ← NEW
│   │   ├── anti-tamper/     ← NEW
│   │   ├── ir-virtualization/ ← NEW (Phase 3)
│   │   └── ...
│   ├── licensing/      ← NEW: ChaosProtector licensing
│   └── include/
├── gradle-plugin/      ← NEW: Gradle integration
├── scripts/            ← Build scripts
├── doc/                ← Documentation
└── PLAN.md             ← This file
```

## Timeline Estimate

- Phase 1 (Rebrand + Licensing): 1-2 weeks
- Phase 2 (New Passes): 2-3 weeks
- Phase 3 (IR Virtualization): 4-6 weeks
- Phase 4 (DEX Protection): 2-3 weeks
- Total: ~3-4 months to full product
