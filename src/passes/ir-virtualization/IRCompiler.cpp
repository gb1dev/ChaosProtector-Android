//
// ChaosProtector Android - IR to VM Bytecode Compiler
//

#include "chaos_android/passes/ir-virtualization/IRCompiler.hpp"
#include "chaos_android/log.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace chaos_android {
namespace vm {

void IRCompiler::emit8(uint8_t Val) { Code.push_back(Val); }
void IRCompiler::emit16(uint16_t Val) {
  Code.push_back(Val & 0xFF);
  Code.push_back((Val >> 8) & 0xFF);
}
void IRCompiler::emit32(uint32_t Val) {
  Code.push_back(Val & 0xFF);
  Code.push_back((Val >> 8) & 0xFF);
  Code.push_back((Val >> 16) & 0xFF);
  Code.push_back((Val >> 24) & 0xFF);
}
void IRCompiler::emit64(uint64_t Val) {
  emit32(Val & 0xFFFFFFFF);
  emit32((Val >> 32) & 0xFFFFFFFF);
}
void IRCompiler::emitOp(Opcode Op) { emit8(static_cast<uint8_t>(Op)); }

void IRCompiler::patchBranch(size_t Offset, uint32_t Target) {
  Code[Offset]     = Target & 0xFF;
  Code[Offset + 1] = (Target >> 8) & 0xFF;
  Code[Offset + 2] = (Target >> 16) & 0xFF;
  Code[Offset + 3] = (Target >> 24) & 0xFF;
}

uint16_t IRCompiler::getLocalIndex(Value *V) {
  auto It = Locals.find(V);
  if (It != Locals.end())
    return It->second;
  uint16_t Idx = NextLocal++;
  Locals[V] = Idx;
  return Idx;
}

bool IRCompiler::canVirtualize(Function &F) {
  if (F.isDeclaration() || F.empty())
    return false;
  if (F.isVarArg())
    return false;
  if (F.hasPersonalityFn())
    return false; // No exception handling support

  // Check all instructions are supported
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (isa<InvokeInst>(I) || isa<ResumeInst>(I) || isa<LandingPadInst>(I))
        return false; // No exception handling
      if (isa<IndirectBrInst>(I))
        return false; // No indirect branches
      if (isa<SwitchInst>(I))
        return false; // TODO: support switch
      if (isa<AtomicRMWInst>(I) || isa<AtomicCmpXchgInst>(I))
        return false; // No atomics
      if (isa<FenceInst>(I))
        return false;
      if (auto *CI = dyn_cast<CallInst>(&I)) {
        if (CI->isInlineAsm())
          return false; // No inline asm
      }
    }
  }
  return true;
}

void IRCompiler::emitPushValue(Value *V) {
  if (auto *CI = dyn_cast<ConstantInt>(V)) {
    uint64_t Val = CI->getZExtValue();
    if (Val <= UINT32_MAX && (int64_t)(int32_t)Val == (int64_t)CI->getSExtValue()) {
      emitOp(OP_PUSH_IMM32);
      emit32((uint32_t)Val);
      pushStack();
    } else {
      emitOp(OP_PUSH_IMM64);
      emit64(Val);
      pushStack();
    }
  } else if (isa<ConstantPointerNull>(V)) {
    emitOp(OP_PUSH_IMM64);
    emit64(0);
    pushStack();
  } else if (isa<UndefValue>(V)) {
    emitOp(OP_PUSH_IMM64);
    emit64(0);
    pushStack();
  } else if (auto *CE = dyn_cast<ConstantExpr>(V)) {
    // Treat constant expressions as opaque values
    // They'll be materialized by LLVM before VM entry
    emitOp(OP_LOAD_LOCAL);
    emit16(getLocalIndex(V));
    pushStack();
  } else if (auto *GV = dyn_cast<GlobalValue>(V)) {
    // Global values are passed as locals (set before VM entry)
    emitOp(OP_LOAD_LOCAL);
    emit16(getLocalIndex(V));
    pushStack();
  } else if (auto *Arg = dyn_cast<Argument>(V)) {
    emitOp(OP_LOAD_ARG);
    emit8(Arg->getArgNo());
    pushStack();
  } else {
    // Must be a previously computed value stored in a local
    emitOp(OP_LOAD_LOCAL);
    emit16(getLocalIndex(V));
    pushStack();
  }
}

bool IRCompiler::emitBinaryOp(BinaryOperator &I) {
  emitPushValue(I.getOperand(0));
  emitPushValue(I.getOperand(1));

  switch (I.getOpcode()) {
  case Instruction::Add:  case Instruction::FAdd: emitOp(I.getType()->isFloatingPointTy() ? OP_FADD : OP_ADD); break;
  case Instruction::Sub:  case Instruction::FSub: emitOp(I.getType()->isFloatingPointTy() ? OP_FSUB : OP_SUB); break;
  case Instruction::Mul:  case Instruction::FMul: emitOp(I.getType()->isFloatingPointTy() ? OP_FMUL : OP_MUL); break;
  case Instruction::SDiv: case Instruction::FDiv: emitOp(I.getType()->isFloatingPointTy() ? OP_FDIV : OP_SDIV); break;
  case Instruction::UDiv: emitOp(OP_UDIV); break;
  case Instruction::SRem: emitOp(OP_SREM); break;
  case Instruction::URem: emitOp(OP_UREM); break;
  case Instruction::And:  emitOp(OP_AND); break;
  case Instruction::Or:   emitOp(OP_OR); break;
  case Instruction::Xor:  emitOp(OP_XOR); break;
  case Instruction::Shl:  emitOp(OP_SHL); break;
  case Instruction::LShr: emitOp(OP_LSHR); break;
  case Instruction::AShr: emitOp(OP_ASHR); break;
  default:
    SWARN("Unsupported binary op: {}", I.getOpcodeName());
    return false;
  }
  popStack(2);
  pushStack(); // result

  emitOp(OP_STORE_LOCAL);
  emit16(getLocalIndex(&I));
  popStack();
  return true;
}

bool IRCompiler::emitICmp(ICmpInst &I) {
  emitPushValue(I.getOperand(0));
  emitPushValue(I.getOperand(1));

  switch (I.getPredicate()) {
  case ICmpInst::ICMP_EQ:  emitOp(OP_CMP_EQ); break;
  case ICmpInst::ICMP_NE:  emitOp(OP_CMP_NE); break;
  case ICmpInst::ICMP_SLT: emitOp(OP_CMP_SLT); break;
  case ICmpInst::ICMP_SLE: emitOp(OP_CMP_SLE); break;
  case ICmpInst::ICMP_SGT: emitOp(OP_CMP_SGT); break;
  case ICmpInst::ICMP_SGE: emitOp(OP_CMP_SGE); break;
  case ICmpInst::ICMP_ULT: emitOp(OP_CMP_ULT); break;
  case ICmpInst::ICMP_ULE: emitOp(OP_CMP_ULE); break;
  case ICmpInst::ICMP_UGT: emitOp(OP_CMP_UGT); break;
  case ICmpInst::ICMP_UGE: emitOp(OP_CMP_UGE); break;
  default: return false;
  }
  popStack(2);
  pushStack();

  emitOp(OP_STORE_LOCAL);
  emit16(getLocalIndex(&I));
  popStack();
  return true;
}

bool IRCompiler::emitFCmp(FCmpInst &I) {
  emitPushValue(I.getOperand(0));
  emitPushValue(I.getOperand(1));

  switch (I.getPredicate()) {
  case FCmpInst::FCMP_OEQ: case FCmpInst::FCMP_UEQ: emitOp(OP_FCMP_OEQ); break;
  case FCmpInst::FCMP_OLT: case FCmpInst::FCMP_ULT: emitOp(OP_FCMP_OLT); break;
  case FCmpInst::FCMP_OGT: case FCmpInst::FCMP_UGT: emitOp(OP_FCMP_OGT); break;
  default:
    // For other predicates, emit OEQ and negate if needed
    emitOp(OP_FCMP_OEQ);
    break;
  }
  popStack(2);
  pushStack();

  emitOp(OP_STORE_LOCAL);
  emit16(getLocalIndex(&I));
  popStack();
  return true;
}

bool IRCompiler::emitLoad(LoadInst &I) {
  emitPushValue(I.getPointerOperand());

  unsigned BitWidth = I.getType()->isPointerTy()
                          ? 64
                          : I.getType()->getIntegerBitWidth();
  switch (BitWidth) {
  case 8:  emitOp(OP_LOAD8); break;
  case 16: emitOp(OP_LOAD16); break;
  case 32: emitOp(OP_LOAD32); break;
  case 64: emitOp(OP_LOAD64); break;
  default:
    if (I.getType()->isFloatingPointTy() || I.getType()->isPointerTy()) {
      emitOp(OP_LOAD64);
    } else {
      SWARN("Unsupported load width: {}", BitWidth);
      return false;
    }
  }
  // Stack: replace ptr with loaded value (net 0)

  emitOp(OP_STORE_LOCAL);
  emit16(getLocalIndex(&I));
  popStack();
  return true;
}

bool IRCompiler::emitStore(StoreInst &I) {
  emitPushValue(I.getPointerOperand());
  emitPushValue(I.getValueOperand());

  Type *ValTy = I.getValueOperand()->getType();
  unsigned BitWidth = ValTy->isPointerTy() ? 64 :
                      ValTy->isIntegerTy() ? ValTy->getIntegerBitWidth() : 64;
  switch (BitWidth) {
  case 8:  emitOp(OP_STORE8); break;
  case 16: emitOp(OP_STORE16); break;
  case 32: emitOp(OP_STORE32); break;
  case 64: emitOp(OP_STORE64); break;
  default: emitOp(OP_STORE64); break;
  }
  popStack(2);
  return true;
}

bool IRCompiler::emitAlloca(AllocaInst &I) {
  // Allocas are pre-allocated before VM entry
  // The local variable holds the alloca pointer
  // (handled externally by the pass)
  return true;
}

bool IRCompiler::emitBranch(BranchInst &I) {
  if (I.isUnconditional()) {
    emitOp(OP_BR);
    size_t PatchLoc = Code.size();
    emit32(0); // placeholder
    UnresolvedBranches.push_back({PatchLoc, I.getSuccessor(0)});
  } else {
    emitPushValue(I.getCondition());
    emitOp(OP_BR_COND);
    size_t PatchTrue = Code.size();
    emit32(0); // placeholder for true target
    popStack();

    emitOp(OP_BR);
    size_t PatchFalse = Code.size();
    emit32(0); // placeholder for false target

    UnresolvedBranches.push_back({PatchTrue, I.getSuccessor(0)});
    UnresolvedBranches.push_back({PatchFalse, I.getSuccessor(1)});
  }
  return true;
}

bool IRCompiler::emitReturn(ReturnInst &I) {
  if (I.getReturnValue()) {
    emitPushValue(I.getReturnValue());
    emitOp(OP_RET);
    popStack();
  } else {
    emitOp(OP_RET_VOID);
  }
  return true;
}

bool IRCompiler::emitCall(CallInst &I) {
  Function *Callee = I.getCalledFunction();
  if (!Callee) {
    // Indirect call
    emitPushValue(I.getCalledOperand());
    for (unsigned i = 0; i < I.arg_size(); i++) {
      emitPushValue(I.getArgOperand(i));
    }
    emitOp(OP_PUSH_IMM32);
    emit32(I.arg_size());
    pushStack();
    emitOp(OP_CALL_INDIRECT);
    popStack(I.arg_size() + 2); // args + nargs + func_ptr
  } else {
    // Direct call - push function address as a local
    emitPushValue(Callee);
    for (unsigned i = 0; i < I.arg_size(); i++) {
      emitPushValue(I.getArgOperand(i));
    }
    emitOp(OP_PUSH_IMM32);
    emit32(I.arg_size());
    pushStack();
    emitOp(OP_CALL_NATIVE);
    popStack(I.arg_size() + 2);
  }

  if (!I.getType()->isVoidTy()) {
    pushStack(); // return value
    emitOp(OP_STORE_LOCAL);
    emit16(getLocalIndex(&I));
    popStack();
  }
  return true;
}

bool IRCompiler::emitGEP(GetElementPtrInst &I) {
  // Simplified GEP: compute byte offset and add to base pointer
  emitPushValue(I.getPointerOperand());

  const DataLayout &DL = I.getModule()->getDataLayout();
  APInt Offset(64, 0);
  if (I.accumulateConstantOffset(DL, Offset)) {
    // Constant offset GEP
    emitOp(OP_PUSH_IMM64);
    emit64(Offset.getZExtValue());
    pushStack();
    emitOp(OP_PTR_ADD);
    popStack(); // net: ptr replaced by ptr+offset
  } else {
    // Variable offset GEP - compute dynamically
    // For simplicity, handle the common case: ptr + idx * element_size
    if (I.getNumIndices() == 1) {
      Value *Idx = I.getOperand(1);
      Type *ElemTy = I.getSourceElementType();
      uint64_t ElemSize = DL.getTypeAllocSize(ElemTy);

      emitPushValue(Idx);
      emitOp(OP_PUSH_IMM64);
      emit64(ElemSize);
      pushStack();
      emitOp(OP_MUL);
      popStack(); // idx * size
      emitOp(OP_PTR_ADD);
      popStack(); // ptr + offset
    } else {
      SWARN("Complex variable GEP not supported in VM");
      return false;
    }
  }

  emitOp(OP_STORE_LOCAL);
  emit16(getLocalIndex(&I));
  popStack();
  return true;
}

bool IRCompiler::emitCast(CastInst &I) {
  emitPushValue(I.getOperand(0));

  switch (I.getOpcode()) {
  case Instruction::SExt:
    if (I.getOperand(0)->getType()->isIntegerTy(8)) emitOp(OP_SEXT8);
    else if (I.getOperand(0)->getType()->isIntegerTy(16)) emitOp(OP_SEXT16);
    else if (I.getOperand(0)->getType()->isIntegerTy(32)) emitOp(OP_SEXT32);
    else { popStack(); emitPushValue(I.getOperand(0)); } // no-op for same width
    break;
  case Instruction::ZExt:
    if (I.getOperand(0)->getType()->isIntegerTy(8)) emitOp(OP_ZEXT8);
    else if (I.getOperand(0)->getType()->isIntegerTy(16)) emitOp(OP_ZEXT16);
    else if (I.getOperand(0)->getType()->isIntegerTy(32)) emitOp(OP_ZEXT32);
    break;
  case Instruction::Trunc:
    if (I.getType()->isIntegerTy(8)) emitOp(OP_TRUNC8);
    else if (I.getType()->isIntegerTy(16)) emitOp(OP_TRUNC16);
    else if (I.getType()->isIntegerTy(32)) emitOp(OP_TRUNC32);
    break;
  case Instruction::SIToFP: emitOp(OP_SITOFP); break;
  case Instruction::FPToSI: emitOp(OP_FPTOSI); break;
  case Instruction::UIToFP: emitOp(OP_UITOFP); break;
  case Instruction::FPToUI: emitOp(OP_FPTOUI); break;
  case Instruction::BitCast:
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
    // No-op in our VM (all values are i64)
    break;
  default:
    SWARN("Unsupported cast: {}", I.getOpcodeName());
    return false;
  }

  emitOp(OP_STORE_LOCAL);
  emit16(getLocalIndex(&I));
  popStack();
  return true;
}

bool IRCompiler::emitSelect(SelectInst &I) {
  emitPushValue(I.getCondition());
  emitPushValue(I.getTrueValue());
  emitPushValue(I.getFalseValue());
  emitOp(OP_SELECT);
  popStack(3);
  pushStack();

  emitOp(OP_STORE_LOCAL);
  emit16(getLocalIndex(&I));
  popStack();
  return true;
}

bool IRCompiler::emitPHI(PHINode &I) {
  // PHI nodes are handled by storing the incoming value in the predecessor
  // block before the branch. At this point, the value should already be
  // in the local variable from the predecessor.
  // No bytecode needed here - handled in branch emission.
  return true;
}

bool IRCompiler::emitInstruction(Instruction &I) {
  if (auto *BO = dyn_cast<BinaryOperator>(&I)) return emitBinaryOp(*BO);
  if (auto *IC = dyn_cast<ICmpInst>(&I)) return emitICmp(*IC);
  if (auto *FC = dyn_cast<FCmpInst>(&I)) return emitFCmp(*FC);
  if (auto *LI = dyn_cast<LoadInst>(&I)) return emitLoad(*LI);
  if (auto *SI = dyn_cast<StoreInst>(&I)) return emitStore(*SI);
  if (auto *AI = dyn_cast<AllocaInst>(&I)) return emitAlloca(*AI);
  if (auto *BI = dyn_cast<BranchInst>(&I)) return emitBranch(*BI);
  if (auto *RI = dyn_cast<ReturnInst>(&I)) return emitReturn(*RI);
  if (auto *CI = dyn_cast<CallInst>(&I)) return emitCall(*CI);
  if (auto *GI = dyn_cast<GetElementPtrInst>(&I)) return emitGEP(*GI);
  if (auto *CA = dyn_cast<CastInst>(&I)) return emitCast(*CA);
  if (auto *SE = dyn_cast<SelectInst>(&I)) return emitSelect(*SE);
  if (auto *PH = dyn_cast<PHINode>(&I)) return emitPHI(*PH);

  SWARN("Unsupported instruction: {}", I.getOpcodeName());
  return false;
}

std::vector<uint8_t> IRCompiler::compile(Function &F) {
  Code.clear();
  Locals.clear();
  BlockOffsets.clear();
  UnresolvedBranches.clear();
  NextLocal = 0;
  CurrentStackDepth = 0;
  MaxStackDepth = 0;

  // Pre-assign locals for arguments
  for (auto &Arg : F.args()) {
    getLocalIndex(&Arg);
  }

  // Pre-assign locals for allocas
  for (auto &BB : F) {
    for (auto &I : BB) {
      if (auto *AI = dyn_cast<AllocaInst>(&I)) {
        getLocalIndex(AI);
      }
    }
  }

  // Pre-assign locals for globals used in the function
  for (auto &BB : F) {
    for (auto &I : BB) {
      for (unsigned i = 0; i < I.getNumOperands(); i++) {
        if (auto *GV = dyn_cast<GlobalValue>(I.getOperand(i))) {
          getLocalIndex(GV);
        }
      }
    }
  }

  // Reserve space for header
  size_t HeaderOffset = Code.size();
  for (int i = 0; i < sizeof(BytecodeHeader); i++)
    emit8(0);

  // Compile each basic block
  for (auto &BB : F) {
    BlockOffsets[&BB] = Code.size();

    // Handle PHI nodes: store incoming values
    for (auto &I : BB) {
      if (!isa<PHINode>(I))
        break;
      // PHIs are resolved at branch time
    }

    for (auto &I : BB) {
      if (isa<PHINode>(I))
        continue; // Skip PHIs (handled by branches)

      if (!emitInstruction(I)) {
        SWARN("Failed to compile instruction in function {}",
              F.getName().str());
        return {};
      }
    }
  }

  // Resolve branch targets
  for (auto &[Offset, BB] : UnresolvedBranches) {
    auto It = BlockOffsets.find(BB);
    if (It == BlockOffsets.end()) {
      SERR("Unresolved branch target");
      return {};
    }
    patchBranch(Offset, It->second);
  }

  // Write header
  BytecodeHeader Header;
  Header.Magic = BYTECODE_MAGIC;
  Header.NumLocals = NextLocal;
  Header.NumArgs = F.arg_size();
  Header.CodeSize = Code.size() - sizeof(BytecodeHeader);
  Header.StackSize = MaxStackDepth + 16; // Extra safety margin

  memcpy(&Code[HeaderOffset], &Header, sizeof(Header));

  SINFO("Compiled {}: {} bytes bytecode, {} locals, {} max stack",
        F.getName().str(), Header.CodeSize, Header.NumLocals,
        Header.StackSize);

  return Code;
}

} // end namespace vm
} // end namespace chaos_android
