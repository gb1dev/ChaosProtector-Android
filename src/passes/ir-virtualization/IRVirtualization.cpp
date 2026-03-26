//
// ChaosProtector Android - IR Virtualization Pass
//
// Pipeline:
// 1. For each function marked for virtualization:
//    a. Compile LLVM IR to VM bytecode via IRCompiler
//    b. Embed bytecode as a global constant in the module
//    c. Replace the function body with a call to __chaos_vm_interpret()
//    d. The interpreter executes the bytecode at runtime
//

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

#include "chaos_android/ObfuscationConfig.hpp"
#include "chaos_android/ProtectionFlags.hpp"
#include "chaos_android/PyConfig.hpp"
#include "chaos_android/log.hpp"
#include "chaos_android/passes/ir-virtualization/IRCompiler.hpp"
#include "chaos_android/passes/ir-virtualization/IRVirtualization.hpp"
#include "chaos_android/passes/ir-virtualization/VMInterpreterEmitter.hpp"
#include "chaos_android/utils.hpp"

using namespace llvm;

namespace chaos_android {

// Create or get the VM interpreter function.
// The interpreter is emitted as LLVM IR directly into the module.
Function *IRVirtualization::getOrCreateInterpreter(Module &M) {
  if (InterpreterFn)
    return InterpreterFn;

  // Check if already exists (e.g. from a previous pass run)
  InterpreterFn = M.getFunction("__chaos_vm_interpret");
  if (InterpreterFn)
    return InterpreterFn;

  // Emit the full interpreter as IR
  InterpreterFn = vm::emitVMInterpreter(M);
  SINFO("[{}] VM interpreter emitted into module", name());
  return InterpreterFn;
}

bool IRVirtualization::virtualizeFunction(Function &F, Module &M) {
  if (!vm::IRCompiler::canVirtualize(F)) {
    SINFO("[{}] Cannot virtualize {} (unsupported instructions)",
          name(), F.getName().str());
    return false;
  }

  // Step 1: Compile IR to bytecode
  vm::IRCompiler Compiler;
  auto Bytecode = Compiler.compile(F);
  if (Bytecode.empty()) {
    SWARN("[{}] Failed to compile {} to bytecode", name(), F.getName().str());
    return false;
  }

  LLVMContext &Ctx = M.getContext();
  auto *Int64Ty = Type::getInt64Ty(Ctx);
  auto *Int32Ty = Type::getInt32Ty(Ctx);
  auto *Int8Ty = Type::getInt8Ty(Ctx);

  // Step 2: Embed bytecode as a global constant
  auto *BytecodeData = ConstantDataArray::get(Ctx, Bytecode);
  auto *BytecodeGlobal = new GlobalVariable(
      M, BytecodeData->getType(), true, GlobalValue::PrivateLinkage,
      BytecodeData, "__chaos_bc_" + F.getName());

  // Step 3: Replace function body
  // Save the original function body and create a new one that calls the interpreter

  // Delete all basic blocks
  while (!F.empty())
    F.begin()->eraseFromParent();

  auto *Entry = BasicBlock::Create(Ctx, "vm_entry", &F);
  IRBuilder<> Builder(Entry);

  // Create args array on stack
  unsigned NumArgs = F.arg_size();
  Value *ArgsArray = nullptr;
  if (NumArgs > 0) {
    ArgsArray = Builder.CreateAlloca(Int64Ty,
                                     ConstantInt::get(Int32Ty, NumArgs));
    unsigned Idx = 0;
    for (auto &Arg : F.args()) {
      Value *Val;
      if (Arg.getType()->isPointerTy()) {
        Val = Builder.CreatePtrToInt(&Arg, Int64Ty);
      } else if (Arg.getType()->isIntegerTy()) {
        if (Arg.getType()->getIntegerBitWidth() < 64) {
          Val = Builder.CreateZExt(&Arg, Int64Ty);
        } else {
          Val = &Arg;
        }
      } else if (Arg.getType()->isFloatingPointTy()) {
        Val = Builder.CreateBitCast(&Arg, Int64Ty);
      } else {
        Val = Builder.CreateBitOrPointerCast(&Arg, Int64Ty);
      }
      auto *Ptr = Builder.CreateGEP(Int64Ty, ArgsArray,
                                     ConstantInt::get(Int32Ty, Idx));
      Builder.CreateStore(Val, Ptr);
      Idx++;
    }
  } else {
    ArgsArray = ConstantPointerNull::get(PointerType::get(Ctx, 0));
  }

  // Call interpreter
  auto *Interp = getOrCreateInterpreter(M);
  auto *Result = Builder.CreateCall(
      Interp,
      {BytecodeGlobal, ArgsArray, ConstantInt::get(Int32Ty, NumArgs)});

  // Return result
  if (F.getReturnType()->isVoidTy()) {
    Builder.CreateRetVoid();
  } else if (F.getReturnType()->isPointerTy()) {
    Builder.CreateRet(Builder.CreateIntToPtr(Result, F.getReturnType()));
  } else if (F.getReturnType()->isIntegerTy()) {
    if (F.getReturnType()->getIntegerBitWidth() < 64) {
      Builder.CreateRet(Builder.CreateTrunc(Result, F.getReturnType()));
    } else {
      Builder.CreateRet(Result);
    }
  } else if (F.getReturnType()->isFloatingPointTy()) {
    Builder.CreateRet(Builder.CreateBitCast(Result, F.getReturnType()));
  } else {
    Builder.CreateRet(Builder.CreateBitOrPointerCast(Result, F.getReturnType()));
  }

  SINFO("[{}] Virtualized {} ({} bytes bytecode)", name(),
        F.getName().str(), Bytecode.size());
  return true;
}

PreservedAnalyses IRVirtualization::run(Module &M,
                                         ModuleAnalysisManager &FAM) {
  if (isModuleGloballyExcluded(&M) || !g_EnableIRVirtualization) {
    return PreservedAnalyses::all();
  }

  SINFO("[{}] Executing on module {}", name(), M.getName());

  PyConfig &Config = PyConfig::instance();
  bool Changed = false;

  // Collect functions to virtualize (don't modify while iterating)
  std::vector<Function *> ToVirtualize;
  for (Function &F : M) {
    if (F.isDeclaration() || F.empty())
      continue;
    if (isFunctionGloballyExcluded(&F))
      continue;
    if (F.getName().starts_with("__chaos"))
      continue;

    // Check config - use defaultConfig to respect include/exclude lists
    if (!Config.getUserConfig()->defaultConfig(&M, &F, {}, {}, {}, 100))
      continue;
    ToVirtualize.push_back(&F);
  }

  for (Function *F : ToVirtualize) {
    Changed |= virtualizeFunction(*F, M);
  }

  SINFO("[{}] Virtualized {}/{} functions", name(),
        Changed ? ToVirtualize.size() : 0, ToVirtualize.size());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace chaos_android
