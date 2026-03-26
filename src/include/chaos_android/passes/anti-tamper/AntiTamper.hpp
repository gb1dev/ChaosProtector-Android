#pragma once

//
// ChaosProtector Android - Anti-Tamper Pass
// Injects runtime integrity verification.
// Computes CRC32 of code sections and verifies at runtime.
//

#include "llvm/IR/PassManager.h"

namespace chaos_android {

struct AntiTamperOpt {
  AntiTamperOpt(bool Value) : Value(Value) {}
  operator bool() const { return Value; }
  bool Value = false;
};

struct AntiTamper : llvm::PassInfoMixin<AntiTamper> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
  static llvm::StringRef name() { return "chaos_android::AntiTamper"; }

private:
  bool runOnModule(llvm::Module &M);
  llvm::Function *createAntiTamperFunction(llvm::Module &M);
};

} // end namespace chaos_android
