//
// ChaosProtector Android - VM Interpreter Emitter
//
// Instead of building the interpreter as complex LLVM IR with dozens of blocks,
// we emit it as a C-like function using IRBuilder. This approach:
// 1. Creates a simple dispatch loop with switch
// 2. Uses helper functions for stack push/pop
// 3. Is easier to maintain and debug
//
// The interpreter function __chaos_vm_interpret is self-contained and
// gets compiled by LLVM into native code alongside the virtualized bytecode.
//

#include "chaos_android/passes/ir-virtualization/VMOpcodes.hpp"
#include "chaos_android/passes/ir-virtualization/VMInterpreterEmitter.hpp"
#include "chaos_android/log.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"

using namespace llvm;

namespace chaos_android {
namespace vm {

// Emit the interpreter as a single function with a simple while-switch loop.
// Since the full IR-based interpreter is complex and error-prone,
// we instead inject a minimal C source as an LLVM IR function.
Function *emitVMInterpreter(Module &M) {
  LLVMContext &Ctx = M.getContext();

  auto *Int8Ty = Type::getInt8Ty(Ctx);
  auto *Int16Ty = Type::getInt16Ty(Ctx);
  auto *Int32Ty = Type::getInt32Ty(Ctx);
  auto *Int64Ty = Type::getInt64Ty(Ctx);
  auto *PtrTy = PointerType::get(Ctx, 0);

  // Signature: i64 __chaos_vm_interpret(ptr bytecode, ptr args, i32 nargs)
  auto *FnTy = FunctionType::get(Int64Ty, {PtrTy, PtrTy, Int32Ty}, false);
  auto *Fn = Function::Create(FnTy, GlobalValue::ExternalLinkage,
                              "__chaos_vm_interpret", M);
  Fn->addFnAttr(Attribute::NoInline);
  Fn->addFnAttr(Attribute::NoUnwind);
  Fn->addFnAttr(Attribute::OptimizeNone); // Prevent LLVM from optimizing the interpreter

  auto *Bytecode = Fn->getArg(0);
  auto *Args = Fn->getArg(1);
  auto *NArgs = Fn->getArg(2);

  // === Entry block: setup ===
  auto *EntryBB = BasicBlock::Create(Ctx, "entry", Fn);
  IRBuilder<> B(EntryBB);

  // Read numLocals and stackSize from header for precise allocation
  auto *NumLocalsRaw = B.CreateLoad(Int16Ty, B.CreateGEP(Int8Ty, Bytecode, B.getInt32(4)));
  auto *NumLocals = B.CreateZExt(NumLocalsRaw, Int32Ty);
  auto *StackSizeRaw = B.CreateLoad(Int32Ty, B.CreateGEP(Int8Ty, Bytecode, B.getInt32(12)));
  // Clamp to reasonable bounds
  auto *NumLocalsClamped = B.CreateSelect(
      B.CreateICmpUGT(NumLocals, B.getInt32(512)), B.getInt32(512), NumLocals);
  auto *StackSizeClamped = B.CreateSelect(
      B.CreateICmpUGT(StackSizeRaw, B.getInt32(128)), B.getInt32(128), StackSizeRaw);
  auto *Locals = B.CreateAlloca(Int64Ty, NumLocalsClamped, "locals");
  auto *ValueStack = B.CreateAlloca(Int64Ty, StackSizeClamped, "vstack");
  auto *SPAlloca = B.CreateAlloca(Int32Ty, nullptr, "sp_alloca");
  auto *IPAlloca = B.CreateAlloca(Int32Ty, nullptr, "ip_alloca");
  auto *RetAlloca = B.CreateAlloca(Int64Ty, nullptr, "ret_alloca");
  auto *RunAlloca = B.CreateAlloca(Int32Ty, nullptr, "run_alloca");

  B.CreateStore(B.getInt32(0), SPAlloca);
  B.CreateStore(B.getInt32(16), IPAlloca); // sizeof(BytecodeHeader)
  B.CreateStore(B.getInt64(0), RetAlloca);
  B.CreateStore(B.getInt32(1), RunAlloca);

  // Copy args to locals[0..nargs-1]
  auto *CopyArgsBB = BasicBlock::Create(Ctx, "copy_args", Fn);
  auto *CopyDoneBB = BasicBlock::Create(Ctx, "copy_done", Fn);
  auto *HasArgs = B.CreateICmpSGT(NArgs, B.getInt32(0));
  B.CreateCondBr(HasArgs, CopyArgsBB, CopyDoneBB);

  B.SetInsertPoint(CopyArgsBB);
  auto *CopyIdx = B.CreatePHI(Int32Ty, 2);
  CopyIdx->addIncoming(B.getInt32(0), EntryBB);
  auto *ArgVal = B.CreateLoad(Int64Ty, B.CreateGEP(Int64Ty, Args, CopyIdx));
  B.CreateStore(ArgVal, B.CreateGEP(Int64Ty, Locals, CopyIdx));
  auto *NextCopyIdx = B.CreateAdd(CopyIdx, B.getInt32(1));
  CopyIdx->addIncoming(NextCopyIdx, CopyArgsBB);
  B.CreateCondBr(B.CreateICmpSLT(NextCopyIdx, NArgs), CopyArgsBB, CopyDoneBB);

  // === Dispatch loop ===
  B.SetInsertPoint(CopyDoneBB);
  auto *LoopBB = BasicBlock::Create(Ctx, "loop", Fn);
  auto *ExitBB = BasicBlock::Create(Ctx, "exit", Fn);
  B.CreateBr(LoopBB);

  B.SetInsertPoint(LoopBB);
  // Check running flag
  auto *IsRunning = B.CreateICmpNE(B.CreateLoad(Int32Ty, RunAlloca), B.getInt32(0));
  auto *FetchBB = BasicBlock::Create(Ctx, "fetch", Fn);
  B.CreateCondBr(IsRunning, FetchBB, ExitBB);

  B.SetInsertPoint(FetchBB);
  // Fetch opcode
  auto *CurIP = B.CreateLoad(Int32Ty, IPAlloca);
  auto *OpByte = B.CreateLoad(Int8Ty, B.CreateGEP(Int8Ty, Bytecode, CurIP));
  B.CreateStore(B.CreateAdd(CurIP, B.getInt32(1)), IPAlloca);

  // Default: stop
  auto *DefaultBB = BasicBlock::Create(Ctx, "op_default", Fn);

  // Create all opcode blocks
  auto makeBB = [&](const char *Name) {
    return BasicBlock::Create(Ctx, Name, Fn);
  };

  // We'll implement a condensed set of opcodes
  auto *BB_push32  = makeBB("push32");
  auto *BB_push64  = makeBB("push64");
  auto *BB_loadl   = makeBB("loadl");
  auto *BB_storel  = makeBB("storel");
  auto *BB_loadarg = makeBB("loadarg");
  auto *BB_add     = makeBB("add");
  auto *BB_sub     = makeBB("sub");
  auto *BB_mul     = makeBB("mul");
  auto *BB_sdiv    = makeBB("sdiv");
  auto *BB_srem    = makeBB("srem");
  auto *BB_and     = makeBB("and_");
  auto *BB_or      = makeBB("or_");
  auto *BB_xor     = makeBB("xor_");
  auto *BB_shl     = makeBB("shl_");
  auto *BB_lshr    = makeBB("lshr_");
  auto *BB_ashr    = makeBB("ashr_");
  auto *BB_cmpeq   = makeBB("cmpeq");
  auto *BB_cmpne   = makeBB("cmpne");
  auto *BB_cmpslt  = makeBB("cmpslt");
  auto *BB_cmpsgt  = makeBB("cmpsgt");
  auto *BB_cmpsle  = makeBB("cmpsle");
  auto *BB_cmpsge  = makeBB("cmpsge");
  auto *BB_ld32    = makeBB("ld32");
  auto *BB_ld64    = makeBB("ld64");
  auto *BB_st32    = makeBB("st32");
  auto *BB_st64    = makeBB("st64");
  auto *BB_br      = makeBB("br_");
  auto *BB_brcond  = makeBB("brcond");
  auto *BB_ret     = makeBB("ret_");
  auto *BB_retvoid = makeBB("retvoid");
  auto *BB_ptradd  = makeBB("ptradd");
  auto *BB_select  = makeBB("select_");
  auto *BB_sext32  = makeBB("sext32");
  auto *BB_zext32  = makeBB("zext32");
  auto *BB_trunc32 = makeBB("trunc32");

  auto *Sw = B.CreateSwitch(OpByte, DefaultBB, 35);
  Sw->addCase(B.getInt8(OP_PUSH_IMM32), BB_push32);
  Sw->addCase(B.getInt8(OP_PUSH_IMM64), BB_push64);
  Sw->addCase(B.getInt8(OP_LOAD_LOCAL), BB_loadl);
  Sw->addCase(B.getInt8(OP_STORE_LOCAL), BB_storel);
  Sw->addCase(B.getInt8(OP_LOAD_ARG), BB_loadarg);
  Sw->addCase(B.getInt8(OP_ADD), BB_add);
  Sw->addCase(B.getInt8(OP_SUB), BB_sub);
  Sw->addCase(B.getInt8(OP_MUL), BB_mul);
  Sw->addCase(B.getInt8(OP_SDIV), BB_sdiv);
  Sw->addCase(B.getInt8(OP_SREM), BB_srem);
  Sw->addCase(B.getInt8(OP_AND), BB_and);
  Sw->addCase(B.getInt8(OP_OR), BB_or);
  Sw->addCase(B.getInt8(OP_XOR), BB_xor);
  Sw->addCase(B.getInt8(OP_SHL), BB_shl);
  Sw->addCase(B.getInt8(OP_LSHR), BB_lshr);
  Sw->addCase(B.getInt8(OP_ASHR), BB_ashr);
  Sw->addCase(B.getInt8(OP_CMP_EQ), BB_cmpeq);
  Sw->addCase(B.getInt8(OP_CMP_NE), BB_cmpne);
  Sw->addCase(B.getInt8(OP_CMP_SLT), BB_cmpslt);
  Sw->addCase(B.getInt8(OP_CMP_SGT), BB_cmpsgt);
  Sw->addCase(B.getInt8(OP_CMP_SLE), BB_cmpsle);
  Sw->addCase(B.getInt8(OP_CMP_SGE), BB_cmpsge);
  Sw->addCase(B.getInt8(OP_LOAD32), BB_ld32);
  Sw->addCase(B.getInt8(OP_LOAD64), BB_ld64);
  Sw->addCase(B.getInt8(OP_STORE32), BB_st32);
  Sw->addCase(B.getInt8(OP_STORE64), BB_st64);
  Sw->addCase(B.getInt8(OP_BR), BB_br);
  Sw->addCase(B.getInt8(OP_BR_COND), BB_brcond);
  Sw->addCase(B.getInt8(OP_RET), BB_ret);
  Sw->addCase(B.getInt8(OP_RET_VOID), BB_retvoid);
  Sw->addCase(B.getInt8(OP_PTR_ADD), BB_ptradd);
  Sw->addCase(B.getInt8(OP_SELECT), BB_select);
  Sw->addCase(B.getInt8(OP_SEXT32), BB_sext32);
  Sw->addCase(B.getInt8(OP_ZEXT32), BB_zext32);
  Sw->addCase(B.getInt8(OP_TRUNC32), BB_trunc32);

  // Helpers (inlined in each block)
  auto rdI32 = [&]() -> Value * {
    auto *ip = B.CreateLoad(Int32Ty, IPAlloca);
    auto *v = B.CreateLoad(Int32Ty, B.CreateGEP(Int8Ty, Bytecode, ip));
    B.CreateStore(B.CreateAdd(ip, B.getInt32(4)), IPAlloca);
    return v;
  };
  auto rdI64 = [&]() -> Value * {
    auto *ip = B.CreateLoad(Int32Ty, IPAlloca);
    auto *v = B.CreateLoad(Int64Ty, B.CreateGEP(Int8Ty, Bytecode, ip));
    B.CreateStore(B.CreateAdd(ip, B.getInt32(8)), IPAlloca);
    return v;
  };
  auto rdI16 = [&]() -> Value * {
    auto *ip = B.CreateLoad(Int32Ty, IPAlloca);
    auto *v = B.CreateLoad(Int16Ty, B.CreateGEP(Int8Ty, Bytecode, ip));
    B.CreateStore(B.CreateAdd(ip, B.getInt32(2)), IPAlloca);
    return v;
  };
  auto rdI8 = [&]() -> Value * {
    auto *ip = B.CreateLoad(Int32Ty, IPAlloca);
    auto *v = B.CreateLoad(Int8Ty, B.CreateGEP(Int8Ty, Bytecode, ip));
    B.CreateStore(B.CreateAdd(ip, B.getInt32(1)), IPAlloca);
    return v;
  };
  auto push = [&](Value *v) {
    auto *sp = B.CreateLoad(Int32Ty, SPAlloca);
    B.CreateStore(v, B.CreateGEP(Int64Ty, ValueStack, sp));
    B.CreateStore(B.CreateAdd(sp, B.getInt32(1)), SPAlloca);
  };
  auto pop = [&]() -> Value * {
    auto *sp = B.CreateSub(B.CreateLoad(Int32Ty, SPAlloca), B.getInt32(1));
    B.CreateStore(sp, SPAlloca);
    return B.CreateLoad(Int64Ty, B.CreateGEP(Int64Ty, ValueStack, sp));
  };

  // === Opcode implementations ===

  B.SetInsertPoint(BB_push32);
  { auto *v = rdI32(); push(B.CreateSExt(v, Int64Ty)); B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_push64);
  { auto *v = rdI64(); push(v); B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_loadl);
  { auto *idx = B.CreateZExt(rdI16(), Int32Ty);
    push(B.CreateLoad(Int64Ty, B.CreateGEP(Int64Ty, Locals, idx)));
    B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_storel);
  { auto *idx = B.CreateZExt(rdI16(), Int32Ty);
    B.CreateStore(pop(), B.CreateGEP(Int64Ty, Locals, idx));
    B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_loadarg);
  { auto *idx = B.CreateZExt(rdI8(), Int32Ty);
    push(B.CreateLoad(Int64Ty, B.CreateGEP(Int64Ty, Args, idx)));
    B.CreateBr(LoopBB); }

  // Binary ops
  auto binop = [&](BasicBlock *bb, Instruction::BinaryOps op) {
    B.SetInsertPoint(bb);
    auto *rhs = pop(); auto *lhs = pop();
    push(B.CreateBinOp(op, lhs, rhs));
    B.CreateBr(LoopBB);
  };
  binop(BB_add, Instruction::Add);
  binop(BB_sub, Instruction::Sub);
  binop(BB_mul, Instruction::Mul);
  binop(BB_sdiv, Instruction::SDiv);
  binop(BB_srem, Instruction::SRem);
  binop(BB_and, Instruction::And);
  binop(BB_or, Instruction::Or);
  binop(BB_xor, Instruction::Xor);
  binop(BB_shl, Instruction::Shl);
  binop(BB_lshr, Instruction::LShr);
  binop(BB_ashr, Instruction::AShr);

  // Comparison ops
  auto cmpop = [&](BasicBlock *bb, ICmpInst::Predicate pred) {
    B.SetInsertPoint(bb);
    auto *rhs = pop(); auto *lhs = pop();
    push(B.CreateZExt(B.CreateICmp(pred, lhs, rhs), Int64Ty));
    B.CreateBr(LoopBB);
  };
  cmpop(BB_cmpeq, ICmpInst::ICMP_EQ);
  cmpop(BB_cmpne, ICmpInst::ICMP_NE);
  cmpop(BB_cmpslt, ICmpInst::ICMP_SLT);
  cmpop(BB_cmpsgt, ICmpInst::ICMP_SGT);
  cmpop(BB_cmpsle, ICmpInst::ICMP_SLE);
  cmpop(BB_cmpsge, ICmpInst::ICMP_SGE);

  // Memory ops
  B.SetInsertPoint(BB_ld32);
  { auto *addr = pop();
    auto *ptr = B.CreateIntToPtr(addr, PointerType::get(Int32Ty, 0));
    push(B.CreateZExt(B.CreateLoad(Int32Ty, ptr), Int64Ty));
    B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_ld64);
  { auto *addr = pop();
    auto *ptr = B.CreateIntToPtr(addr, PointerType::get(Int64Ty, 0));
    push(B.CreateLoad(Int64Ty, ptr));
    B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_st32);
  { auto *val = pop(); auto *addr = pop();
    auto *ptr = B.CreateIntToPtr(addr, PointerType::get(Int32Ty, 0));
    B.CreateStore(B.CreateTrunc(val, Int32Ty), ptr);
    B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_st64);
  { auto *val = pop(); auto *addr = pop();
    auto *ptr = B.CreateIntToPtr(addr, PointerType::get(Int64Ty, 0));
    B.CreateStore(val, ptr);
    B.CreateBr(LoopBB); }

  // Control flow
  B.SetInsertPoint(BB_br);
  { auto *target = rdI32(); B.CreateStore(target, IPAlloca); B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_brcond);
  { auto *target = rdI32(); auto *cond = pop();
    auto *taken = BasicBlock::Create(Ctx, "taken", Fn);
    auto *nottaken = BasicBlock::Create(Ctx, "nottaken", Fn);
    B.CreateCondBr(B.CreateICmpNE(cond, B.getInt64(0)), taken, nottaken);
    B.SetInsertPoint(taken);
    B.CreateStore(target, IPAlloca);
    B.CreateBr(LoopBB);
    B.SetInsertPoint(nottaken);
    B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_ret);
  { B.CreateStore(pop(), RetAlloca);
    B.CreateStore(B.getInt32(0), RunAlloca);
    B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_retvoid);
  { B.CreateStore(B.getInt64(0), RetAlloca);
    B.CreateStore(B.getInt32(0), RunAlloca);
    B.CreateBr(LoopBB); }

  // Pointer add
  B.SetInsertPoint(BB_ptradd);
  { auto *off = pop(); auto *base = pop();
    push(B.CreateAdd(base, off)); B.CreateBr(LoopBB); }

  // Select
  B.SetInsertPoint(BB_select);
  { auto *fv = pop(); auto *tv = pop(); auto *c = pop();
    push(B.CreateSelect(B.CreateICmpNE(c, B.getInt64(0)), tv, fv));
    B.CreateBr(LoopBB); }

  // Type ops
  B.SetInsertPoint(BB_sext32);
  { auto *v = pop(); push(B.CreateSExt(B.CreateTrunc(v, Int32Ty), Int64Ty));
    B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_zext32);
  { auto *v = pop(); push(B.CreateZExt(B.CreateTrunc(v, Int32Ty), Int64Ty));
    B.CreateBr(LoopBB); }

  B.SetInsertPoint(BB_trunc32);
  { auto *v = pop(); push(B.CreateZExt(B.CreateTrunc(v, Int32Ty), Int64Ty));
    B.CreateBr(LoopBB); }

  // Default: stop
  B.SetInsertPoint(DefaultBB);
  B.CreateStore(B.getInt32(0), RunAlloca);
  B.CreateBr(LoopBB);

  // Exit
  B.SetInsertPoint(ExitBB);
  B.CreateRet(B.CreateLoad(Int64Ty, RetAlloca));

  SINFO("VM interpreter emitted: {} basic blocks", Fn->size());
  return Fn;
}

} // end namespace vm
} // end namespace chaos_android
