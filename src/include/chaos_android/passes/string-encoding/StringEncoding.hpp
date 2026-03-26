#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <variant>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/PassManager.h"
#include "llvm/TargetParser/Triple.h"

#include "chaos_android/passes/string-encoding/Routines.h"
#include "chaos_android/passes/string-encoding/StringEncodingOpt.hpp"

// Forward declarations
namespace llvm {
class ConstantDataSequential;
class GlobalVariable;
class CallInst;
} // end namespace llvm

namespace chaos_android {

struct ObfuscationConfig;
class Jitter;

// See https://obfuscator.re/chaos_android/passes/strings-encoding for details.
struct StringEncoding : llvm::PassInfoMixin<StringEncoding> {
  using KeyBufferTy = std::vector<uint8_t>;
  using KeyIntTy = uint64_t;
  using KeyTy = std::variant<std::monostate, KeyBufferTy, KeyIntTy>;

  enum EncodingTy {
    None = 0,
    Local,
    Global,
    Replace,
  };

  struct EncodingInfo {
    EncodingInfo() = delete;
    EncodingInfo(EncodingTy Ty) : Type(Ty) {};

    EncodingTy Type = EncodingTy::None;
    KeyTy Key;
    std::unique_ptr<llvm::Module> TM;
    EncRoutineFn *EncodeFn;
  };

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return true; }

  bool encodeStrings(llvm::Function &F, ObfuscationConfig &UserConfig);
  bool injectDecoding(llvm::Instruction &I, llvm::Use &Op,
                      llvm::GlobalVariable &G,
                      llvm::ConstantDataSequential &Data,
                      const EncodingInfo &Info);
  bool injectDecodingLocally(llvm::Instruction &I, llvm::Use &Op,
                             llvm::GlobalVariable &G,
                             llvm::ConstantDataSequential &Data,
                             const EncodingInfo &Info);
  llvm::CallInst *createDecodingTrampoline(
      llvm::GlobalVariable &G, llvm::Use &EncPtr, llvm::Instruction *NewPt,
      uint64_t KeyValI64, uint64_t Size, const StringEncoding::EncodingInfo &EI,
      bool IsLocalToFunction = false);
  bool process(llvm::Instruction &I, llvm::Use &Op, llvm::GlobalVariable &G,
               llvm::ConstantDataSequential &Data, StringEncodingOpt &Opt);
  bool processReplace(llvm::Use &Op, llvm::GlobalVariable &G,
                      llvm::ConstantDataSequential &Data,
                      StringEncOptReplace &Rep);
  bool processGlobal(llvm::Use &Op, llvm::GlobalVariable &G,
                     llvm::ConstantDataSequential &Data);
  bool processLocal(llvm::Instruction &I, llvm::Use &Op,
                    llvm::GlobalVariable &G,
                    llvm::ConstantDataSequential &Data);
  bool processArrayOfStrings(llvm::Instruction &CurrentI, llvm::Use &Op,
                             llvm::ConstantArray *CA, llvm::GlobalVariable *GV,
                             ObfuscationConfig &);

  inline EncodingInfo *getEncoding(const llvm::GlobalVariable &GV) {
    if (auto It = GVarEncInfo.find(&GV); It != GVarEncInfo.end())
      return &It->second;
    return nullptr;
  }

private:
  void genRoutines(const llvm::Triple &Triple, EncodingInfo &EI,
                   llvm::LLVMContext &Ctx);
  void annotateRoutine(llvm::Module &M);
  llvm::Constant *reconstructConstantArray(llvm::ConstantArray *CA);

  std::vector<llvm::CallInst *> ToInline;
  std::vector<llvm::Function *> Ctors;
  llvm::SmallSet<llvm::GlobalVariable *, 10> Obf;
  llvm::DenseMap<llvm::ConstantDataSequential *, std::vector<uint8_t>> KeyMap;
  llvm::DenseMap<llvm::GlobalVariable *, EncodingInfo> GVarEncInfo;
  llvm::DenseMap<llvm::GlobalVariable *, llvm::GlobalVariable *>
      OriginalToDecoded;
};

} // end namespace chaos_android
