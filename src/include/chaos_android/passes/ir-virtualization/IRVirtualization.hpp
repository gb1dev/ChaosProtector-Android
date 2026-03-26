#pragma once

//
// ChaosProtector Android - IR Virtualization Pass
//
// Converts selected functions' LLVM IR into custom VM bytecode,
// then replaces the function body with a call to the embedded VM interpreter.
//
// This is the most powerful obfuscation technique:
// - Original IR logic is converted to opaque bytecode
// - Custom stack-based VM interpreter executes the bytecode
// - Each build can use different opcode assignments (polymorphic)
// - Static analysis tools cannot decompile VM-protected functions
//

#include "llvm/IR/PassManager.h"

namespace chaos_android {

struct IRVirtualizationOpt {
  IRVirtualizationOpt(bool Value) : Value(Value) {}
  operator bool() const { return Value; }
  bool Value = false;
};

struct IRVirtualization : llvm::PassInfoMixin<IRVirtualization> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
  static llvm::StringRef name() { return "chaos_android::IRVirtualization"; }

private:
  bool virtualizeFunction(llvm::Function &F, llvm::Module &M);
  llvm::Function *getOrCreateInterpreter(llvm::Module &M);
  llvm::Function *InterpreterFn = nullptr;
};

} // end namespace chaos_android
