#pragma once

//
// ChaosProtector Android - IR to VM Bytecode Compiler
//
// Converts LLVM IR instructions into VM bytecode.
// Each function is compiled independently into a bytecode blob.
//

#include <vector>
#include <map>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "chaos_android/passes/ir-virtualization/VMOpcodes.hpp"

namespace chaos_android {
namespace vm {

class IRCompiler {
public:
  // Compile a function to bytecode. Returns empty vector on failure.
  std::vector<uint8_t> compile(llvm::Function &F);

  // Check if a function can be virtualized
  static bool canVirtualize(llvm::Function &F);

private:
  // Emit bytecode for a single instruction
  bool emitInstruction(llvm::Instruction &I);

  // Emit bytecode for specific instruction types
  bool emitBinaryOp(llvm::BinaryOperator &I);
  bool emitICmp(llvm::ICmpInst &I);
  bool emitFCmp(llvm::FCmpInst &I);
  bool emitLoad(llvm::LoadInst &I);
  bool emitStore(llvm::StoreInst &I);
  bool emitAlloca(llvm::AllocaInst &I);
  bool emitBranch(llvm::BranchInst &I);
  bool emitReturn(llvm::ReturnInst &I);
  bool emitCall(llvm::CallInst &I);
  bool emitGEP(llvm::GetElementPtrInst &I);
  bool emitCast(llvm::CastInst &I);
  bool emitSelect(llvm::SelectInst &I);
  bool emitPHI(llvm::PHINode &I);
  void emitPhiResolution(llvm::BasicBlock *CurrentBB, llvm::BasicBlock *TargetBB);

  // Push a value reference onto the virtual stack
  void emitPushValue(llvm::Value *V);

  // Bytecode emission helpers
  void emit8(uint8_t Val);
  void emit16(uint16_t Val);
  void emit32(uint32_t Val);
  void emit64(uint64_t Val);
  void emitOp(Opcode Op);

  // Patch a branch target
  void patchBranch(size_t Offset, uint32_t Target);

  // Get/assign local variable index for a value
  uint16_t getLocalIndex(llvm::Value *V);

  // Current bytecode output
  std::vector<uint8_t> Code;

  // Local variable mapping: Value -> local index
  std::map<llvm::Value *, uint16_t> Locals;
  uint16_t NextLocal = 0;

  // Basic block -> bytecode offset mapping
  std::map<llvm::BasicBlock *, uint32_t> BlockOffsets;

  // Unresolved branch targets: (bytecode offset of target field, basic block)
  std::vector<std::pair<size_t, llvm::BasicBlock *>> UnresolvedBranches;

  // Stack depth tracking
  uint32_t CurrentStackDepth = 0;
  uint32_t MaxStackDepth = 0;

  void pushStack(uint32_t N = 1) {
    CurrentStackDepth += N;
    if (CurrentStackDepth > MaxStackDepth)
      MaxStackDepth = CurrentStackDepth;
  }
  void popStack(uint32_t N = 1) {
    CurrentStackDepth -= N;
  }
};

} // end namespace vm
} // end namespace chaos_android
