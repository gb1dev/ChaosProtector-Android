//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/Module.h"
#include "llvm/TargetParser/Triple.h"

#include "chaos_android/log.hpp"
#include "chaos_android/passes/logger-bind/LoggerBind.hpp"

namespace chaos_android {

llvm::PreservedAnalyses LoggerBind::run(llvm::Module &M,
                                        llvm::ModuleAnalysisManager &) {
  auto ModuleName =
      M.getModuleIdentifier().empty() ? "Module" : M.getModuleIdentifier();
  llvm::Triple TT(M.getTargetTriple());
  auto TargetArch =
      TT.getArchName().empty() ? "Unknown" : TT.getArchName().str();

  Logger::BindModule(ModuleName, TargetArch);
  return llvm::PreservedAnalyses::all();
}

} // end namespace chaos_android
