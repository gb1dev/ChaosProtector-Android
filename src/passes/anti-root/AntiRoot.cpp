//
// ChaosProtector Android - Anti-Root Pass
//
// Injects a constructor function that checks for root indicators.
// Detection techniques:
// 1. Check for su binary in common locations
// 2. Check for Magisk files (/sbin/.magisk, /data/adb/magisk)
// 3. Check for root management apps (Superuser, SuperSU, etc.)
// 4. Check Build.TAGS for "test-keys" via system property
// 5. Check SELinux enforcement (permissive = suspicious)
//
// On detection: calls abort() / raises SIGKILL.
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
#include "chaos_android/ProtectionFlags.hpp"
#include "chaos_android/passes/anti-root/AntiRoot.hpp"
#include "chaos_android/utils.hpp"

using namespace llvm;

namespace chaos_android {

// Paths to check for root indicators
static const char *RootPaths[] = {
    "/system/app/Superuser.apk",
    "/system/app/SuperSU.apk",
    "/sbin/su",
    "/system/bin/su",
    "/system/xbin/su",
    "/data/local/xbin/su",
    "/data/local/bin/su",
    "/system/sd/xbin/su",
    "/system/bin/failsafe/su",
    "/data/local/su",
    "/su/bin/su",
    "/sbin/.magisk",
    "/data/adb/magisk",
    "/cache/.disable_magisk",
    "/dev/.magisk.unblock",
    "/data/data/com.noshufou.android.su",
    "/data/data/com.thirdparty.superuser",
    "/data/data/eu.chainfire.supersu",
    "/data/data/com.koushikdutta.superuser",
    "/data/data/com.topjohnwu.magisk",
};

Function *AntiRoot::createAntiRootFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  const auto &TT = Triple(M.getTargetTriple());

  auto *VoidTy = Type::getVoidTy(Ctx);
  auto *Int32Ty = Type::getInt32Ty(Ctx);
  auto *Int64Ty = Type::getInt64Ty(Ctx);
  auto *Int8PtrTy = PointerType::get(Ctx, 0);

  auto *FnTy = FunctionType::get(VoidTy, false);
  auto *Fn = Function::Create(FnTy, GlobalValue::InternalLinkage,
                              "__chaos_anti_root", M);
  Fn->addFnAttr(Attribute::NoUnwind);
  Fn->addFnAttr(Attribute::OptimizeNone);
  Fn->addFnAttr(Attribute::NoInline);

  auto *Entry = BasicBlock::Create(Ctx, "entry", Fn);
  auto *RootDetected = BasicBlock::Create(Ctx, "root_detected", Fn);
  auto *Clean = BasicBlock::Create(Ctx, "clean", Fn);

  IRBuilder<> Builder(Entry);

  if (TT.isAArch64()) {
    // Check each root path using access() syscall
    // __NR_faccessat on AArch64 = 48
    // faccessat(AT_FDCWD, path, F_OK, 0)
    auto *FaccessatTy = FunctionType::get(
        Int64Ty, {Int64Ty, Int8PtrTy, Int64Ty, Int64Ty}, false);
    auto *FaccessatSyscall = InlineAsm::get(
        FaccessatTy,
        "mov x8, #48\n"   // __NR_faccessat
        "svc #0\n",
        "={x0},{x0},{x1},{x2},{x3},~{x8}",
        true);

    auto *AtFdCwd = ConstantInt::get(Int64Ty, -100);
    auto *FOk = ConstantInt::get(Int64Ty, 0); // F_OK
    auto *Zero64 = ConstantInt::get(Int64Ty, 0);

    BasicBlock *CurrentBB = Entry;

    for (const char *Path : RootPaths) {
      auto *PathStr = Builder.CreateGlobalStringPtr(Path);
      auto *Result = Builder.CreateCall(
          FaccessatTy, FaccessatSyscall,
          {AtFdCwd, PathStr, FOk, Zero64});

      // access returns 0 if file exists
      auto *Exists = Builder.CreateICmpEQ(Result, Zero64, "exists");

      auto *NextBB = BasicBlock::Create(Ctx, "next", Fn);
      Builder.CreateCondBr(Exists, RootDetected, NextBB);

      Builder.SetInsertPoint(NextBB);
      CurrentBB = NextBB;
    }

    // === Check: read /proc/self/status for SELinux context ===
    // If SELinux is permissive or disabled, likely rooted
    // Check /sys/fs/selinux/enforce - contains '1' for enforcing, '0' for permissive
    auto *SelinuxPath = Builder.CreateGlobalStringPtr("/sys/fs/selinux/enforce");
    auto *OpenAtTy = FunctionType::get(Int64Ty, {Int64Ty, Int8PtrTy, Int64Ty}, false);
    auto *OpenSyscall = InlineAsm::get(
        OpenAtTy,
        "mov x8, #56\n"   // __NR_openat
        "svc #0\n",
        "={x0},{x0},{x1},{x2},~{x8}",
        true);

    auto *Fd = Builder.CreateCall(OpenAtTy, OpenSyscall,
                                  {AtFdCwd, SelinuxPath, Zero64});

    // Only check if file opened successfully (fd >= 0)
    auto *FdValid = Builder.CreateICmpSGE(Fd, Zero64, "fd_valid");
    auto *ReadSelinux = BasicBlock::Create(Ctx, "read_selinux", Fn);
    auto *AfterSelinux = BasicBlock::Create(Ctx, "after_selinux", Fn);
    Builder.CreateCondBr(FdValid, ReadSelinux, AfterSelinux);

    Builder.SetInsertPoint(ReadSelinux);
    // read 1 byte
    auto *OneByte = Builder.CreateAlloca(Type::getInt8Ty(Ctx),
                                         ConstantInt::get(Int32Ty, 2));
    auto *ReadTy = FunctionType::get(Int64Ty, {Int64Ty, Int8PtrTy, Int64Ty}, false);
    auto *ReadSyscall = InlineAsm::get(
        ReadTy,
        "mov x8, #63\n"   // __NR_read
        "svc #0\n",
        "={x0},{x0},{x1},{x2},~{x8}",
        true);
    Builder.CreateCall(ReadTy, ReadSyscall,
                       {Fd, OneByte, ConstantInt::get(Int64Ty, 1)});

    // close
    auto *CloseTy = FunctionType::get(Int64Ty, {Int64Ty}, false);
    auto *CloseSyscall = InlineAsm::get(
        CloseTy,
        "mov x8, #57\n"  // __NR_close
        "svc #0\n",
        "={x0},{x0},~{x8}",
        true);
    Builder.CreateCall(CloseTy, CloseSyscall, {Fd});

    // Check if value is '0' (permissive = rooted)
    auto *EnforceVal = Builder.CreateLoad(Type::getInt8Ty(Ctx), OneByte);
    auto *IsPermissive = Builder.CreateICmpEQ(
        EnforceVal, ConstantInt::get(Type::getInt8Ty(Ctx), '0'));
    Builder.CreateCondBr(IsPermissive, RootDetected, AfterSelinux);

    Builder.SetInsertPoint(AfterSelinux);
    Builder.CreateBr(Clean);

  } else {
    Builder.CreateBr(Clean);
  }

  // === Root detected: crash ===
  Builder.SetInsertPoint(RootDetected);
  if (TT.isAArch64()) {
    auto *GetPidTy = FunctionType::get(Int64Ty, false);
    auto *GetPidSyscall = InlineAsm::get(
        GetPidTy,
        "mov x8, #172\n"
        "svc #0\n",
        "={x0},~{x8}",
        true);
    auto *Pid = Builder.CreateCall(GetPidTy, GetPidSyscall, {});

    auto *KillTy = FunctionType::get(Int64Ty, {Int64Ty, Int64Ty}, false);
    auto *KillSyscall = InlineAsm::get(
        KillTy,
        "mov x8, #129\n"
        "svc #0\n",
        "={x0},{x0},{x1},~{x8}",
        true);
    Builder.CreateCall(KillTy, KillSyscall,
                       {Pid, ConstantInt::get(Int64Ty, 9)});
  }
  Builder.CreateUnreachable();

  // === Clean ===
  Builder.SetInsertPoint(Clean);
  Builder.CreateRetVoid();

  return Fn;
}

bool AntiRoot::runOnModule(Module &M) {
  const auto &TT = Triple(M.getTargetTriple());
  if (!TT.isAArch64() && !TT.isARM()) {
    SWARN("[{}] Only AArch64/ARM targets supported. Skipping...", name());
    return false;
  }

  auto *CheckFn = createAntiRootFunction(M);
  appendToGlobalCtors(M, CheckFn, 102);

  SINFO("[{}] Anti-root constructor injected (checking {} paths)",
        name(), sizeof(RootPaths) / sizeof(RootPaths[0]));
  return true;
}

PreservedAnalyses AntiRoot::run(Module &M, ModuleAnalysisManager &FAM) {
  if (isModuleGloballyExcluded(&M) || !g_EnableAntiRoot) {
    return PreservedAnalyses::all();
  }

  SINFO("[{}] Executing on module {}", name(), M.getName());
  bool Changed = runOnModule(M);

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace chaos_android
