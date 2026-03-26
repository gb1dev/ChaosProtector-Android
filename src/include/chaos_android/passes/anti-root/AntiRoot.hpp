#pragma once

//
// ChaosProtector Android - Anti-Root Pass
// Detects rooted devices at runtime.
// Checks: su binary, Magisk, SELinux permissive, root apps, test-keys.
//

#include "llvm/IR/PassManager.h"

namespace chaos_android {

struct AntiRootOpt {
  AntiRootOpt(bool Value) : Value(Value) {}
  operator bool() const { return Value; }
  bool Value = false;
};

struct AntiRoot : llvm::PassInfoMixin<AntiRoot> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
  static llvm::StringRef name() { return "chaos_android::AntiRoot"; }

private:
  bool runOnModule(llvm::Module &M);
  llvm::Function *createAntiRootFunction(llvm::Module &M);
};

} // end namespace chaos_android
