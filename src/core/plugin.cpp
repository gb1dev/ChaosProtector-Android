//
// ChaosProtector Android - LLVM Pass Plugin Entry Point
//
// Supports two configuration modes:
// 1. YAML-only (chaos-android.yml with protections: section) - no Python needed
// 2. Python config (backwards compatible with O-MVLL configs)
//

#include <dlfcn.h>

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Threading.h"
#include "llvm/Support/YAMLParser.h"
#include "llvm/Support/YAMLTraits.h"

#include "chaos_android/PyConfig.hpp"
#include "chaos_android/YamlObfuscationConfig.hpp"
#include "chaos_android/Licensing.hpp"
#include "chaos_android/jitter.hpp"
#include "chaos_android/log.hpp"
#include "chaos_android/passes.hpp"
#include "chaos_android/utils.hpp"

using namespace llvm;

static llvm::once_flag InitializePluginFlag;

// Global: if true, we use YAML-only config (no Python)
static bool UseYamlOnly = false;
static std::unique_ptr<chaos_android::YamlObfuscationConfig> g_YamlCfg;
static std::string FoundYamlPath;

template <> struct yaml::MappingTraits<chaos_android::YamlConfig> {
  static void mapping(IO &IO, chaos_android::YamlConfig &Config) {
    IO.mapOptional("PYTHONPATH", Config.PythonPath, "");
    IO.mapOptional("CONFIG", Config.ConfigPath, "");
  }
};

static std::string expandAbsPath(StringRef Path, StringRef Base) {
  if (sys::path::is_absolute(Path)) {
    if (!sys::fs::exists(Path))
      SWARN("Absolute path does not exist");
    return Path.str();
  }

  SmallString<256> YConfigAbs = Base;
  sys::path::append(YConfigAbs, Path);
  if (!sys::fs::exists(YConfigAbs))
    SWARN("Relative path does not exist");

  return YConfigAbs.str().str();
}

// Check if YAML file has a 'protections:' key (YAML-only mode)
static bool hasProtectionsSection(const std::string &Path) {
  auto Buffer = MemoryBuffer::getFile(Path);
  if (!Buffer)
    return false;
  // Simple check: look for "protections:" in the file
  return (*Buffer)->getBuffer().contains("protections:");
}

static bool findYamlFile(std::string Dir) {
  while (true) {
    SmallString<256> YPath = StringRef(Dir);
    sys::path::append(YPath, chaos_android::PyConfig::YamlFile);
    if (sys::fs::exists(YPath)) {
      FoundYamlPath = YPath.str().str();
      SINFO("Found chaos-android.yml at {}", FoundYamlPath);
      return true;
    }
    if (sys::path::has_parent_path(Dir)) {
      Dir = sys::path::parent_path(Dir);
    } else {
      return false;
    }
  }
}

static bool loadYamlConfig(StringRef Dir, StringRef FileName) {
  SmallString<256> YConfig = Dir;
  sys::path::append(YConfig, FileName);
  if (!sys::fs::exists(YConfig))
    return false;

  SINFO("Loading chaos-android.yml from {}", YConfig.str());
  auto Buffer = MemoryBuffer::getFile(YConfig);
  if (!Buffer) {
    SERR("Cannot read '{}': {}", YConfig.str(), Buffer.getError().message());
    return false;
  }

  yaml::Input Input(**Buffer);
  chaos_android::YamlConfig Config;
  Input >> Config;

  Config.PythonPath = expandAbsPath(Config.PythonPath, Dir);
  Config.ConfigPath = expandAbsPath(Config.ConfigPath, Dir);

  chaos_android::PyConfig::YConfig = Config;
  return true;
}

static bool findYamlConfig(std::string Dir) {
  while (true) {
    SINFO("Looking for chaos-android.yml in {}", Dir);
    if (loadYamlConfig(Dir, chaos_android::PyConfig::YamlFile))
      return true;
    if (sys::path::has_parent_path(Dir)) {
      Dir = sys::path::parent_path(Dir);
    } else {
      return false;
    }
  }
}

void chaos_android::initYamlConfig() {
  SmallString<256> CurrentPath;
  if (auto Err = sys::fs::current_path(CurrentPath)) {
    SERR("Cannot determine the current path: '{}'", Err.message());
  } else {
    if (findYamlConfig(CurrentPath.str().str()))
      return;
  }

  Dl_info Info;
  if (!dladdr((void *)findYamlConfig, &Info)) {
    SERR("Cannot determine plugin file path");
  } else {
    SmallString<256> PluginDir = StringRef(Info.dli_fname);
    sys::path::remove_filename(PluginDir);
    if (findYamlConfig(PluginDir.str().str()))
      return;
  }

  SINFO("Could not find chaos-android.yml");
}

static void initializePluginOnce() {
  // Step 1: Find the YAML file
  SmallString<256> CurrentPath;
  bool FoundYaml = false;
  if (!sys::fs::current_path(CurrentPath)) {
    FoundYaml = findYamlFile(CurrentPath.str().str());
  }

  if (!FoundYaml) {
    Dl_info Info;
    if (dladdr((void *)findYamlFile, &Info)) {
      SmallString<256> PluginDir = StringRef(Info.dli_fname);
      sys::path::remove_filename(PluginDir);
      FoundYaml = findYamlFile(PluginDir.str().str());
    }
  }

  // Step 2: Check if YAML has protections: section (YAML-only mode)
  if (FoundYaml && hasProtectionsSection(FoundYamlPath)) {
    SINFO("Using YAML-only configuration mode (no Python needed)");
    UseYamlOnly = true;

    chaos_android::YamlProtectionConfig ProtCfg;
    if (chaos_android::parseYamlProtectionConfig(FoundYamlPath, ProtCfg)) {
      // Validate license and apply tier restrictions
      auto Tier = chaos_android::LicenseValidator::validate(ProtCfg.LicenseKey);
      chaos_android::LicenseValidator::applyTierRestrictions(ProtCfg, Tier);
      ProtCfg.Tier = Tier;

      g_YamlCfg =
          std::make_unique<chaos_android::YamlObfuscationConfig>(ProtCfg);
      // Set as external config so passes can use PyConfig::instance().getUserConfig()
      chaos_android::PyConfig::setExternalConfig(g_YamlCfg.get());
      SINFO("ChaosProtector Android v2.0.0 initialized ({} tier)",
            chaos_android::LicenseValidator::tierName(Tier));
    } else {
      SERR("Failed to parse YAML protection config");
      UseYamlOnly = false;
    }
  }

  // Step 3: Fall back to Python mode
  if (!UseYamlOnly) {
    SINFO("Using Python configuration mode");
    chaos_android::initYamlConfig();
    chaos_android::initPythonpath();
    auto &Instance = chaos_android::PyConfig::instance();
    SINFO("Found ChaosProtector Android config at: {}",
          Instance.configPath());
  }
}

PassPluginLibraryInfo getChaosAndroidPluginInfo() {
  chaos_android::Logger::set_level(spdlog::level::level_enum::debug);
  llvm::call_once(InitializePluginFlag, initializePluginOnce);

  return {LLVM_PLUGIN_API_VERSION, "ChaosProtector-Android", "2.0.0",
          [](PassBuilder &PB) {
            try {
              PB.registerPipelineEarlySimplificationEPCallback(
                  [&](ModulePassManager &MPM, OptimizationLevel Opt) {
                    MPM.addPass(chaos_android::LoggerBind());
                    MPM.addPass(chaos_android::AntiHook());
                    MPM.addPass(chaos_android::FunctionOutline());
                    MPM.addPass(chaos_android::StringEncoding());
                    MPM.addPass(chaos_android::OpaqueFieldAccess());
                    MPM.addPass(chaos_android::BasicBlockDuplicate());
                    MPM.addPass(chaos_android::ControlFlowFlattening());
                    MPM.addPass(chaos_android::BreakControlFlow());
                    MPM.addPass(chaos_android::OpaqueConstants());
                    MPM.addPass(chaos_android::Arithmetic());
#ifdef CHAOS_ANDROID_EXPERIMENTAL
                    MPM.addPass(chaos_android::ObjCleaner());
#endif
                    MPM.addPass(chaos_android::IndirectCall());
                    MPM.addPass(chaos_android::IndirectBranch());
                    MPM.addPass(chaos_android::Cleaning());
                    return true;
                  });
            } catch (const std::exception &Exc) {
              chaos_android::fatalError(Exc.what());
            }
          }};
}

// Force export the plugin entry point (visibility("default") must override
// the compile-time -fvisibility=hidden flag)
#pragma GCC visibility push(default)
extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
  return getChaosAndroidPluginInfo();
}
#pragma GCC visibility pop

// Expose the YAML config for passes to use
namespace chaos_android {
ObfuscationConfig *getActiveConfig() {
  if (UseYamlOnly && g_YamlCfg)
    return g_YamlCfg.get();
  return PyConfig::instance().getUserConfig();
}
} // namespace chaos_android
