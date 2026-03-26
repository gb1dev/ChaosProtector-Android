#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/PassManager.h"

namespace chaos_android {

struct Cleaning : llvm::PassInfoMixin<Cleaning> {
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &FAM);
};

} // end namespace chaos_android
