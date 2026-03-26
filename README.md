# ChaosProtector Android

LLVM-based code obfuscation for Android native libraries (.so) and iOS.

Based on [O-MVLL](https://github.com/open-obfuscator/o-mvll) (Apache 2.0).

## Features

- **String Encryption** - 5 encoding strategies for string literals
- **Control Flow Flattening** - State machine dispatch
- **MBA Arithmetic** - Mixed Boolean-Arithmetic substitution
- **Opaque Constants** - Hide constants in opaque predicates
- **Opaque Field Access** - Obfuscate struct field loads
- **Function Outlining** - Extract blocks into new functions
- **Indirect Branches/Calls** - Jump/call table indirection
- **Basic Block Duplication** - Clone blocks with dead code
- **Anti-Hook** - Frida hooking detection
- **Break Control Flow** - Insert bogus branches

## Quick Start

```yaml
# chaos-android.yml (project root)
protections:
  string_encryption: true
  control_flow_flattening: true
  opaque_constants: true
  anti_hook: true
```

```bash
# Build with NDK
clang -fpass-plugin=libChaosAndroid.so -target aarch64-linux-android app.c -o app
```

## Gradle Integration

```groovy
plugins {
    id 'com.chaosprotector.android' version '2.0.0'
}

chaosProtector {
    licenseKey = "CP-XXXX-XXXX-XXXX-XXXX"
    protections {
        stringEncryption = true
        controlFlowFlattening = true
    }
}
```

## Build

See [PLAN.md](PLAN.md) for development roadmap.

## License

Apache License 2.0. See [LICENSE](LICENSE).
