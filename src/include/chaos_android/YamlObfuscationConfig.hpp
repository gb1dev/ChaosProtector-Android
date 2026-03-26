#pragma once

//
// ChaosProtector Android - YAML-only ObfuscationConfig
// No Python required for basic usage.
//

#include <string>
#include <vector>
#include <set>

#include "chaos_android/ObfuscationConfig.hpp"

namespace chaos_android {

enum class LicenseTier {
  Free = 0,
  Basic = 1,
  Pro = 2,
};

struct YamlProtectionConfig {
  // Pass enables
  bool StringEncryption = false;
  bool ControlFlowFlattening = false;
  bool OpaqueConstants = false;
  bool Arithmetic = false;
  bool AntiHook = false;
  bool IndirectBranch = false;
  bool IndirectCall = false;
  bool BreakControlFlow = false;
  bool BasicBlockDuplicate = false;
  bool FunctionOutline = false;
  bool OpaqueFieldAccess = false;

  // Phase 2 passes
  bool AntiDebug = false;
  bool AntiRoot = false;
  bool AntiTamper = false;
  bool IRVirtualization = false;

  // Exclude/include lists
  std::vector<std::string> Exclude;
  std::vector<std::string> Include;

  // Probability (0-100) for probabilistic passes
  int Probability = 50;

  // License
  std::string LicenseKey;
  LicenseTier Tier = LicenseTier::Free;
};

class YamlObfuscationConfig : public ObfuscationConfig {
public:
  explicit YamlObfuscationConfig(const YamlProtectionConfig &Config)
      : Cfg(Config) {}

  StringEncodingOpt obfuscateString(llvm::Module *M, llvm::Function *F,
                                    const std::string &Str) override;

  StructAccessOpt obfuscateStructAccess(llvm::Module *M, llvm::Function *F,
                                        llvm::StructType *S) override;

  VarAccessOpt obfuscateVariableAccess(llvm::Module *M, llvm::Function *F,
                                       llvm::GlobalVariable *S) override;

  BreakControlFlowOpt breakControlFlow(llvm::Module *M,
                                       llvm::Function *F) override;

  ControlFlowFlatteningOpt controlFlowGraphFlattening(llvm::Module *M,
                                                       llvm::Function *F) override;

  OpaqueConstantsOpt obfuscateConstants(llvm::Module *M,
                                        llvm::Function *F) override;

  ArithmeticOpt obfuscateArithmetics(llvm::Module *M,
                                     llvm::Function *F) override;

  AntiHookOpt antiHooking(llvm::Module *M, llvm::Function *F) override;

  IndirectBranchOpt indirectBranch(llvm::Module *M,
                                   llvm::Function *F) override;

  IndirectCallOpt indirectCall(llvm::Module *M, llvm::Function *F) override;

  BasicBlockDuplicateOpt basicBlockDuplicate(llvm::Module *M,
                                              llvm::Function *F) override;

  FunctionOutlineOpt functionOutline(llvm::Module *M,
                                     llvm::Function *F) override;

  bool defaultConfig(llvm::Module *M, llvm::Function *F,
                     const std::vector<std::string> &ModuleExcludes,
                     const std::vector<std::string> &FunctionExcludes,
                     const std::vector<std::string> &FunctionIncludes,
                     int Probability) override;

  bool hasReportDiffOverride() override { return false; }
  void reportDiff(const std::string &Pass, const std::string &Original,
                  const std::string &Obfuscated) override {}

private:
  bool isFunctionIncluded(llvm::Function *F) const;
  YamlProtectionConfig Cfg;
};

// Parse chaos-android.yml with full protection config
bool parseYamlProtectionConfig(const std::string &Path,
                               YamlProtectionConfig &Out);

} // end namespace chaos_android
