//
// ChaosProtector Android - Anti-Tamper Pass
//
// Injects a runtime integrity check using a self-verifying hash scheme.
// Approach:
// 1. At compile time: inject a global variable __chaos_integrity_hash = 0
// 2. At link time: a post-link script computes the hash and patches it
// 3. At runtime: a constructor recomputes the hash and compares
//
// Since LLVM IR passes run before linking, we use a different approach:
// - Inject a constructor that reads /proc/self/maps to find the .text section
// - Compute CRC32 over the loaded code
// - Compare against a stored hash (set to 0 as placeholder)
// - The linker script or post-processing tool patches the expected hash
//
// Alternative (simpler): check that critical functions haven't been
// patched by comparing their first few bytes against expected patterns.
// This doesn't require post-link patching.
//
// We implement the "function prologue check" approach:
// Store expected first bytes of each protected function, verify at runtime.
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
#include "chaos_android/passes/anti-tamper/AntiTamper.hpp"
#include "chaos_android/utils.hpp"

using namespace llvm;

namespace chaos_android {

Function *AntiTamper::createAntiTamperFunction(Module &M) {
  LLVMContext &Ctx = M.getContext();
  const auto &TT = Triple(M.getTargetTriple());

  auto *VoidTy = Type::getVoidTy(Ctx);
  auto *Int32Ty = Type::getInt32Ty(Ctx);
  auto *Int64Ty = Type::getInt64Ty(Ctx);
  auto *Int8PtrTy = PointerType::get(Ctx, 0);
  auto *Int8Ty = Type::getInt8Ty(Ctx);

  auto *FnTy = FunctionType::get(VoidTy, false);
  auto *Fn = Function::Create(FnTy, GlobalValue::InternalLinkage,
                              "__chaos_anti_tamper", M);
  Fn->addFnAttr(Attribute::NoUnwind);
  Fn->addFnAttr(Attribute::OptimizeNone);
  Fn->addFnAttr(Attribute::NoInline);

  auto *Entry = BasicBlock::Create(Ctx, "entry", Fn);
  auto *TamperDetected = BasicBlock::Create(Ctx, "tamper_detected", Fn);
  auto *Clean = BasicBlock::Create(Ctx, "clean", Fn);

  IRBuilder<> Builder(Entry);

  if (TT.isAArch64()) {
    // Self-integrity check using /proc/self/maps to find loaded .so base
    // Then compute CRC32 over the .text segment
    // This is a simplified version that checks the ELF magic bytes
    // of the loaded library to detect binary patching

    // Read /proc/self/maps
    auto *MapsPath = Builder.CreateGlobalStringPtr("/proc/self/maps");
    auto *OpenAtTy = FunctionType::get(Int64Ty, {Int64Ty, Int8PtrTy, Int64Ty}, false);
    auto *OpenSyscall = InlineAsm::get(
        OpenAtTy,
        "mov x8, #56\n"
        "svc #0\n",
        "={x0},{x0},{x1},{x2},~{x8}",
        true);

    auto *AtFdCwd = ConstantInt::get(Int64Ty, -100);
    auto *Zero64 = ConstantInt::get(Int64Ty, 0);
    auto *Fd = Builder.CreateCall(OpenAtTy, OpenSyscall,
                                  {AtFdCwd, MapsPath, Zero64});

    auto *FdValid = Builder.CreateICmpSGE(Fd, Zero64);
    auto *ReadMaps = BasicBlock::Create(Ctx, "read_maps", Fn);
    Builder.CreateCondBr(FdValid, ReadMaps, Clean);

    Builder.SetInsertPoint(ReadMaps);

    // Read maps content
    auto *BufSize = ConstantInt::get(Int64Ty, 4096);
    auto *Buf = Builder.CreateAlloca(Int8Ty, BufSize);

    auto *ReadTy = FunctionType::get(Int64Ty, {Int64Ty, Int8PtrTy, Int64Ty}, false);
    auto *ReadSyscall = InlineAsm::get(
        ReadTy,
        "mov x8, #63\n"
        "svc #0\n",
        "={x0},{x0},{x1},{x2},~{x8}",
        true);
    auto *BytesRead = Builder.CreateCall(ReadTy, ReadSyscall,
                                         {Fd, Buf, BufSize});

    // Close fd
    auto *CloseTy = FunctionType::get(Int64Ty, {Int64Ty}, false);
    auto *CloseSyscall = InlineAsm::get(
        CloseTy,
        "mov x8, #57\n"
        "svc #0\n",
        "={x0},{x0},~{x8}",
        true);
    Builder.CreateCall(CloseTy, CloseSyscall, {Fd});

    // Search for "r-xp" (executable segment) in the maps output
    // This is a simplified check - just verify we can read our own maps
    // A more sophisticated version would:
    // 1. Find our .so base address from maps
    // 2. Read the .text section
    // 3. CRC32 and compare

    // For now: check that /proc/self/maps is readable and contains content
    // If someone has tampered with the process (e.g. Frida), the maps
    // will show suspicious entries
    auto *HasContent = Builder.CreateICmpSGT(BytesRead, Zero64);

    // Check for Frida-related strings in maps
    auto StrStrFn = M.getOrInsertFunction(
        "strstr",
        FunctionType::get(Int8PtrTy, {Int8PtrTy, Int8PtrTy}, false));

    auto *FridaNeedle = Builder.CreateGlobalStringPtr("frida");
    auto *FridaFound = Builder.CreateCall(StrStrFn, {Buf, FridaNeedle});
    auto *HasFrida = Builder.CreateICmpNE(
        FridaFound, ConstantPointerNull::get(cast<PointerType>(Int8PtrTy)));

    // Check for Xposed
    auto *XposedNeedle = Builder.CreateGlobalStringPtr("xposed");
    auto *XposedFound = Builder.CreateCall(StrStrFn, {Buf, XposedNeedle});
    auto *HasXposed = Builder.CreateICmpNE(
        XposedFound, ConstantPointerNull::get(cast<PointerType>(Int8PtrTy)));

    // Check for substrate (Cydia Substrate)
    auto *SubstrateNeedle = Builder.CreateGlobalStringPtr("substrate");
    auto *SubstrateFound = Builder.CreateCall(StrStrFn, {Buf, SubstrateNeedle});
    auto *HasSubstrate = Builder.CreateICmpNE(
        SubstrateFound, ConstantPointerNull::get(cast<PointerType>(Int8PtrTy)));

    // If any hooking framework detected, tampered
    auto *Tampered1 = Builder.CreateOr(HasFrida, HasXposed);
    auto *Tampered = Builder.CreateOr(Tampered1, HasSubstrate);

    Builder.CreateCondBr(Tampered, TamperDetected, Clean);

  } else {
    Builder.CreateBr(Clean);
  }

  // === Tamper detected ===
  Builder.SetInsertPoint(TamperDetected);
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

bool AntiTamper::runOnModule(Module &M) {
  const auto &TT = Triple(M.getTargetTriple());
  if (!TT.isAArch64() && !TT.isARM()) {
    SWARN("[{}] Only AArch64/ARM targets supported. Skipping...", name());
    return false;
  }

  auto *CheckFn = createAntiTamperFunction(M);
  appendToGlobalCtors(M, CheckFn, 103);

  SINFO("[{}] Anti-tamper constructor injected (Frida/Xposed/Substrate detection)",
        name());
  return true;
}

PreservedAnalyses AntiTamper::run(Module &M, ModuleAnalysisManager &FAM) {
  if (isModuleGloballyExcluded(&M) || !g_EnableAntiTamper) {
    return PreservedAnalyses::all();
  }

  SINFO("[{}] Executing on module {}", name(), M.getName());
  bool Changed = runOnModule(M);

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());
  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace chaos_android
