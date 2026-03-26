#pragma once

//
// ChaosProtector Android - Anti-Debug Pass
// Injects runtime debugger detection checks into protected functions.
// Techniques: ptrace self-trace, /proc/self/status TracerPid, timing checks.
//

#include "llvm/IR/PassManager.h"

namespace chaos_android {

struct AntiDebugOpt {
  AntiDebugOpt(bool Value) : Value(Value) {}
  operator bool() const { return Value; }
  bool Value = false;
};

struct AntiDebug : llvm::PassInfoMixin<AntiDebug> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
  static llvm::StringRef name() { return "chaos_android::AntiDebug"; }

private:
  bool runOnModule(llvm::Module &M);
  llvm::Function *createAntiDebugFunction(llvm::Module &M);
};

} // end namespace chaos_android
