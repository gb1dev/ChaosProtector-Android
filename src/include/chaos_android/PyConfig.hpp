#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <string>
#include <memory>
#include <vector>

#include "chaos_android/passes/ObfuscationOpt.hpp"

// Forward declarations
namespace pybind11 {
class module_;
} // end namespace pybind11

// Forward declarations
namespace llvm {
class Module;
class Function;
class StructType;
} // end namespace llvm

namespace chaos_android {

struct ObfuscationConfig;

struct YamlConfig {
  std::string PythonPath;
  std::string ConfigPath;
};

void initPythonpath();
void initYamlConfig();

class PyConfig {
public:
  static PyConfig &instance();
  ObfuscationConfig *getUserConfig();
  std::string configPath();

  // Set an external config (used by YAML-only mode to bypass Python)
  static void setExternalConfig(ObfuscationConfig *Cfg) { ExternalConfig = Cfg; }
  static bool hasExternalConfig() { return ExternalConfig != nullptr; }

  static constexpr auto DefaultFileName = "chaos_config";
  static constexpr auto EnvKey = "CHAOS_ANDROID_CONFIG";
  static constexpr auto PyEnv_Key = "CHAOS_ANDROID_PYTHONPATH";
  static constexpr auto YamlFile = "chaos-android.yml";

  static inline YamlConfig YConfig;
  static inline ObfuscationConfig *ExternalConfig = nullptr;

  PyConfig(const PyConfig &) = delete;
  PyConfig &operator=(const PyConfig &) = delete;

private:
  PyConfig();
  ~PyConfig();

  std::unique_ptr<pybind11::module_> Mod;
  std::unique_ptr<pybind11::module_> CoreMod;
  std::string ModulePath;
};

} // end namespace chaos_android
