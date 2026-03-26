//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "chaos_android/chaos_config.hpp"
#include "chaos_android/passes.hpp"

namespace chaos_android {

ChaosConfig Config;

void initDefaultConfig() {
  Config.Passes = {
      AntiHook::name().str(),
      StringEncoding::name().str(),

      OpaqueFieldAccess::name().str(),
      ControlFlowFlattening::name().str(),
      BreakControlFlow::name().str(),

      OpaqueConstants::name().str(),
      Arithmetic::name().str(),
      IndirectBranch::name().str(),
      IndirectCall::name().str(),
      BasicBlockDuplicate::name().str(),
      FunctionOutline::name().str(),

      // Last pass.
      Cleaning::name().str(),
  };

  Config.Cleaning = true;
  Config.InlineJniWrappers = true;
  Config.ShuffleFunctions = true;
  Config.GlobalModuleExclude.clear();
  Config.GlobalFunctionExclude.clear();
  Config.ProbabilitySeed = 1;
  Config.OutputFolder = "";
}

} // end namespace chaos_android
