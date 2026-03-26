#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <string>
#include <vector>

namespace chaos_android {

struct ChaosConfig {
  std::vector<std::string> Passes;
  std::vector<std::string> GlobalModuleExclude;
  std::vector<std::string> GlobalFunctionExclude;
  std::string OutputFolder;
  bool Cleaning;
  bool ShuffleFunctions;
  bool InlineJniWrappers;
  int ProbabilitySeed;
};

// Defined in chaos_config.cpp.
extern ChaosConfig Config;
void initDefaultConfig();

} // end namespace chaos_android
