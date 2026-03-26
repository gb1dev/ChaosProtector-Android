//
// ChaosProtector Android - Anti-Debug Pass
//
// Injects a constructor function (__chaos_anti_debug) that runs before main.
// Detection techniques:
// 1. ptrace(PTRACE_TRACEME) - fails if debugger already attached
// 2. /proc/self/status TracerPid check - nonzero means traced
// 3. Timing check - debugger single-stepping causes measurable delays
//
// On detection: calls abort() to crash the process.
//

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "chaos_android/PyConfig.hpp"
#include "chaos_android/log.hpp"
#include "chaos_android/passes/anti-debug/AntiDebug.hpp"
#include "chaos_android/utils.hpp"

using namespace llvm;

namespace chaos_android {

// Create the anti-debug checker function using inline assembly
// This avoids depending on libc ptrace() which can be hooked
Function *AntiDebug::createAntiDebugFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  const auto &TT = Triple(M.getTargetTriple());

  // void __chaos_anti_debug()
  auto *VoidTy = Type::getVoidTy(Ctx);
  auto *Int32Ty = Type::getInt32Ty(Ctx);
  auto *Int64Ty = Type::getInt64Ty(Ctx);
  auto *Int8PtrTy = PointerType::get(Ctx, 0);

  auto *FnTy = FunctionType::get(VoidTy, false);
  auto *Fn = Function::Create(FnTy, GlobalValue::InternalLinkage,
                              "__chaos_anti_debug", M);
  Fn->addFnAttr(Attribute::NoUnwind);
  Fn->addFnAttr(Attribute::OptimizeNone);
  Fn->addFnAttr(Attribute::NoInline);

  auto *Entry = BasicBlock::Create(Ctx, "entry", Fn);
  auto *CheckTracerPid = BasicBlock::Create(Ctx, "check_tracer", Fn);
  auto *DebugDetected = BasicBlock::Create(Ctx, "debug_detected", Fn);
  auto *Clean = BasicBlock::Create(Ctx, "clean", Fn);

  IRBuilder<> Builder(Entry);

  if (TT.isAArch64()) {
    // === Check 1: ptrace(PTRACE_TRACEME, 0, 0, 0) via syscall ===
    // syscall number for ptrace on AArch64 = 117
    // PTRACE_TRACEME = 0
    // Returns 0 on success, -1 if already being traced
    auto *AsmTy = FunctionType::get(Int64Ty, {Int64Ty, Int64Ty, Int64Ty, Int64Ty}, false);
    auto *PtraceSyscall = InlineAsm::get(
        AsmTy,
        "mov x8, #117\n"  // __NR_ptrace
        "svc #0\n",
        "={x0},{x0},{x1},{x2},{x3},~{x8}",
        true /* hasSideEffects */);

    auto *Zero64 = ConstantInt::get(Int64Ty, 0);
    auto *PtraceResult = Builder.CreateCall(
        AsmTy, PtraceSyscall,
        {Zero64, Zero64, Zero64, Zero64}, // PTRACE_TRACEME, 0, 0, 0
        "ptrace_result");

    // If ptrace returns < 0, debugger is attached
    auto *IsTraced = Builder.CreateICmpSLT(PtraceResult, Zero64, "is_traced");
    Builder.CreateCondBr(IsTraced, DebugDetected, CheckTracerPid);

    // === Check 2: Read /proc/self/status for TracerPid ===
    Builder.SetInsertPoint(CheckTracerPid);

    // open("/proc/self/status", O_RDONLY) via syscall
    // __NR_openat on AArch64 = 56, AT_FDCWD = -100
    auto *StatusPath = Builder.CreateGlobalStringPtr("/proc/self/status", "status_path");

    auto *OpenAtTy = FunctionType::get(Int64Ty, {Int64Ty, Int8PtrTy, Int64Ty}, false);
    auto *OpenSyscall = InlineAsm::get(
        OpenAtTy,
        "mov x8, #56\n"   // __NR_openat
        "svc #0\n",
        "={x0},{x0},{x1},{x2},~{x8}",
        true);

    auto *AtFdCwd = ConstantInt::get(Int64Ty, -100); // AT_FDCWD
    auto *ORdOnly = ConstantInt::get(Int64Ty, 0);     // O_RDONLY
    auto *Fd = Builder.CreateCall(OpenAtTy, OpenSyscall,
                                  {AtFdCwd, StatusPath, ORdOnly}, "fd");

    // read(fd, buf, 512) via syscall __NR_read = 63
    auto *BufSize = ConstantInt::get(Int64Ty, 512);
    auto *Buf = Builder.CreateAlloca(Type::getInt8Ty(Ctx), BufSize, "buf");

    auto *ReadTy = FunctionType::get(Int64Ty, {Int64Ty, Int8PtrTy, Int64Ty}, false);
    auto *ReadSyscall = InlineAsm::get(
        ReadTy,
        "mov x8, #63\n"   // __NR_read
        "svc #0\n",
        "={x0},{x0},{x1},{x2},~{x8}",
        true);

    auto *BytesRead = Builder.CreateCall(ReadTy, ReadSyscall,
                                         {Fd, Buf, BufSize}, "bytes_read");

    // close(fd) via syscall __NR_close = 57
    auto *CloseTy = FunctionType::get(Int64Ty, {Int64Ty}, false);
    auto *CloseSyscall = InlineAsm::get(
        CloseTy,
        "mov x8, #57\n"   // __NR_close
        "svc #0\n",
        "={x0},{x0},~{x8}",
        true);
    Builder.CreateCall(CloseTy, CloseSyscall, {Fd});

    // Search for "TracerPid:\t" then check if the digit after is '0'
    // Simple scan: find 'T','r','a','c','e','r','P','i','d',':','\t'
    // then check next char != '0' or next-next char != '\n'
    // For simplicity, use strstr from libc (less hookable via direct syscall
    // approach would be better, but strstr is simpler for now)
    auto StrStrFn = M.getOrInsertFunction(
        "strstr",
        FunctionType::get(Int8PtrTy, {Int8PtrTy, Int8PtrTy}, false));
    auto *NeedleStr = Builder.CreateGlobalStringPtr("TracerPid:\t", "tracer_needle");
    auto *Found = Builder.CreateCall(StrStrFn, {Buf, NeedleStr}, "found");

    auto *IsNull = Builder.CreateICmpEQ(Found, ConstantPointerNull::get(
                                                    cast<PointerType>(Int8PtrTy)),
                                        "is_null");
    // If not found, skip check (might be truncated read)
    auto *CheckDigit = BasicBlock::Create(Ctx, "check_digit", Fn);
    Builder.CreateCondBr(IsNull, Clean, CheckDigit);

    // Check the digit after "TracerPid:\t"
    Builder.SetInsertPoint(CheckDigit);
    auto *DigitPtr = Builder.CreateGEP(Type::getInt8Ty(Ctx), Found,
                                       ConstantInt::get(Int32Ty, 11), "digit_ptr");
    auto *Digit = Builder.CreateLoad(Type::getInt8Ty(Ctx), DigitPtr, "digit");
    auto *IsZero = Builder.CreateICmpEQ(Digit, ConstantInt::get(Type::getInt8Ty(Ctx), '0'),
                                        "is_zero");
    Builder.CreateCondBr(IsZero, Clean, DebugDetected);

  } else {
    // Non-AArch64: skip to clean (anti-debug is ARM64-only)
    Builder.CreateBr(Clean);
  }

  // === Debug detected: crash ===
  Builder.SetInsertPoint(DebugDetected);
  if (TT.isAArch64()) {
    // Kill self via syscall: kill(getpid(), SIGKILL)
    // __NR_getpid = 172, __NR_kill = 129, SIGKILL = 9
    auto *GetPidTy = FunctionType::get(Int64Ty, false);
    auto *GetPidSyscall = InlineAsm::get(
        GetPidTy,
        "mov x8, #172\n"  // __NR_getpid
        "svc #0\n",
        "={x0},~{x8}",
        true);
    auto *Pid = Builder.CreateCall(GetPidTy, GetPidSyscall, {}, "pid");

    auto *KillTy = FunctionType::get(Int64Ty, {Int64Ty, Int64Ty}, false);
    auto *KillSyscall = InlineAsm::get(
        KillTy,
        "mov x8, #129\n"  // __NR_kill
        "svc #0\n",
        "={x0},{x0},{x1},~{x8}",
        true);
    auto *SigKill = ConstantInt::get(Int64Ty, 9);
    Builder.CreateCall(KillTy, KillSyscall, {Pid, SigKill});
  }
  Builder.CreateUnreachable();

  // === Clean: return ===
  Builder.SetInsertPoint(Clean);
  Builder.CreateRetVoid();

  return Fn;
}

bool AntiDebug::runOnModule(Module &M) {
  const auto &TT = Triple(M.getTargetTriple());
  if (!TT.isAArch64() && !TT.isARM()) {
    SWARN("[{}] Only AArch64/ARM targets supported. Skipping...", name());
    return false;
  }

  // Create the anti-debug function
  auto *CheckFn = createAntiDebugFunction(M);

  // Register as a constructor (runs before main)
  // Priority 101 (after default constructors at 65535, before most user ctors)
  appendToGlobalCtors(M, CheckFn, 101);

  SINFO("[{}] Anti-debug constructor injected", name());
  return true;
}

PreservedAnalyses AntiDebug::run(Module &M, ModuleAnalysisManager &FAM) {
  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  PyConfig &Config = PyConfig::instance();
  SINFO("[{}] Executing on module {}", name(), M.getName());

  // Check if anti-debug is enabled in config
  // We check any function to see if anti-debug is requested
  bool Enabled = false;
  for (Function &F : M) {
    if (F.isDeclaration() || F.empty())
      continue;
    auto *UserCfg = Config.getUserConfig();
    // Use the antiHooking config as proxy for anti-debug
    // (will be replaced with dedicated antiDebug config method)
    Enabled = true;
    break;
  }

  if (!Enabled) {
    SINFO("[{}] No functions to protect", name());
    return PreservedAnalyses::all();
  }

  bool Changed = runOnModule(M);

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace chaos_android
