//
// ChaosProtector Android - YAML-only ObfuscationConfig implementation
//

#include "chaos_android/YamlObfuscationConfig.hpp"
#include "chaos_android/log.hpp"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/YAMLParser.h"

using namespace llvm;

namespace chaos_android {

// Check if function name matches any pattern in a list
static bool matchesPattern(StringRef Name, const std::string &Pattern) {
  if (Pattern.empty())
    return false;
  if (Pattern == "*")
    return true;
  if (Pattern.back() == '*') {
    StringRef Prefix(Pattern.data(), Pattern.size() - 1);
    return Name.starts_with(Prefix);
  }
  if (Pattern.front() == '*') {
    StringRef Suffix(Pattern.data() + 1, Pattern.size() - 1);
    return Name.ends_with(Suffix);
  }
  return Name == Pattern;
}

bool YamlObfuscationConfig::isFunctionIncluded(llvm::Function *F) const {
  if (!F)
    return false;
  StringRef Name = F->getName();

  if (!Cfg.Include.empty()) {
    for (const auto &Pat : Cfg.Include) {
      if (matchesPattern(Name, Pat))
        return true;
    }
    return false;
  }

  for (const auto &Pat : Cfg.Exclude) {
    if (matchesPattern(Name, Pat))
      return false;
  }
  return true;
}

StringEncodingOpt
YamlObfuscationConfig::obfuscateString(llvm::Module *M, llvm::Function *F,
                                       const std::string &Str) {
  if (!Cfg.StringEncryption || !isFunctionIncluded(F))
    return StringEncodingOpt(StringEncOptSkip());
  return StringEncodingOpt(StringEncOptGlobal());
}

StructAccessOpt
YamlObfuscationConfig::obfuscateStructAccess(llvm::Module *M,
                                             llvm::Function *F,
                                             llvm::StructType *S) {
  if (!Cfg.OpaqueFieldAccess || !isFunctionIncluded(F))
    return StructAccessOpt(false);
  return StructAccessOpt(true);
}

VarAccessOpt
YamlObfuscationConfig::obfuscateVariableAccess(llvm::Module *M,
                                               llvm::Function *F,
                                               llvm::GlobalVariable *S) {
  if (!Cfg.OpaqueFieldAccess || !isFunctionIncluded(F))
    return VarAccessOpt(false);
  return VarAccessOpt(true);
}

BreakControlFlowOpt
YamlObfuscationConfig::breakControlFlow(llvm::Module *M, llvm::Function *F) {
  if (!Cfg.BreakControlFlow || !isFunctionIncluded(F))
    return BreakControlFlowOpt(false);
  return BreakControlFlowOpt(true);
}

ControlFlowFlatteningOpt
YamlObfuscationConfig::controlFlowGraphFlattening(llvm::Module *M,
                                                   llvm::Function *F) {
  if (!Cfg.ControlFlowFlattening || !isFunctionIncluded(F))
    return ControlFlowFlatteningOpt(false);
  return ControlFlowFlatteningOpt(true);
}

OpaqueConstantsOpt
YamlObfuscationConfig::obfuscateConstants(llvm::Module *M, llvm::Function *F) {
  if (!Cfg.OpaqueConstants || !isFunctionIncluded(F))
    return OpaqueConstantsOpt(OpaqueConstantsSkip());
  return OpaqueConstantsOpt(OpaqueConstantsBool(true));
}

ArithmeticOpt YamlObfuscationConfig::obfuscateArithmetics(llvm::Module *M,
                                                           llvm::Function *F) {
  if (!Cfg.Arithmetic || !isFunctionIncluded(F))
    return ArithmeticOpt(false);
  return ArithmeticOpt(true);
}

AntiHookOpt YamlObfuscationConfig::antiHooking(llvm::Module *M,
                                                llvm::Function *F) {
  if (!Cfg.AntiHook || !isFunctionIncluded(F))
    return AntiHookOpt(false);
  return AntiHookOpt(true);
}

IndirectBranchOpt YamlObfuscationConfig::indirectBranch(llvm::Module *M,
                                                         llvm::Function *F) {
  if (!Cfg.IndirectBranch || !isFunctionIncluded(F))
    return std::nullopt;
  return IndirectBranchConfig(true);
}

IndirectCallOpt YamlObfuscationConfig::indirectCall(llvm::Module *M,
                                                     llvm::Function *F) {
  if (!Cfg.IndirectCall || !isFunctionIncluded(F))
    return std::nullopt;
  return IndirectCallConfig(true);
}

BasicBlockDuplicateOpt
YamlObfuscationConfig::basicBlockDuplicate(llvm::Module *M,
                                            llvm::Function *F) {
  if (!Cfg.BasicBlockDuplicate || !isFunctionIncluded(F))
    return BasicBlockDuplicateOpt(BasicBlockDuplicateSkip());
  return BasicBlockDuplicateOpt(
      BasicBlockDuplicateWithProbability(Cfg.Probability));
}

FunctionOutlineOpt
YamlObfuscationConfig::functionOutline(llvm::Module *M, llvm::Function *F) {
  if (!Cfg.FunctionOutline || !isFunctionIncluded(F))
    return FunctionOutlineOpt(FunctionOutlineSkip());
  return FunctionOutlineOpt(FunctionOutlineWithProbability(Cfg.Probability));
}

bool YamlObfuscationConfig::defaultConfig(
    llvm::Module *M, llvm::Function *F,
    const std::vector<std::string> &ModuleExcludes,
    const std::vector<std::string> &FunctionExcludes,
    const std::vector<std::string> &FunctionIncludes, int Probability) {
  return isFunctionIncluded(F);
}

// YAML parsing helpers
static bool getYamlBool(yaml::Node *N) {
  if (auto *Scalar = dyn_cast<yaml::ScalarNode>(N)) {
    SmallString<8> Storage;
    StringRef Val = Scalar->getValue(Storage);
    return Val == "true" || Val == "yes" || Val == "1";
  }
  return false;
}

static int getYamlInt(yaml::Node *N) {
  if (auto *Scalar = dyn_cast<yaml::ScalarNode>(N)) {
    SmallString<8> Storage;
    StringRef Val = Scalar->getValue(Storage);
    int Result = 0;
    Val.getAsInteger(10, Result);
    return Result;
  }
  return 0;
}

static std::string getYamlString(yaml::Node *N) {
  if (auto *Scalar = dyn_cast<yaml::ScalarNode>(N)) {
    SmallString<256> Storage;
    return Scalar->getValue(Storage).str();
  }
  return "";
}

static std::vector<std::string> getYamlStringList(yaml::Node *N) {
  std::vector<std::string> Result;
  if (auto *Seq = dyn_cast<yaml::SequenceNode>(N)) {
    for (auto &Item : *Seq) {
      if (auto *Scalar = dyn_cast<yaml::ScalarNode>(&Item)) {
        SmallString<256> Storage;
        Result.push_back(Scalar->getValue(Storage).str());
      }
    }
  }
  return Result;
}

bool parseYamlProtectionConfig(const std::string &Path,
                               YamlProtectionConfig &Out) {
  auto Buffer = MemoryBuffer::getFile(Path);
  if (!Buffer) {
    SERR("Cannot read YAML config '{}': {}", Path,
         Buffer.getError().message());
    return false;
  }

  llvm::SourceMgr SM;
  yaml::Stream Stream((*Buffer)->getBuffer(), SM);
  auto DocIt = Stream.begin();
  if (DocIt == Stream.end())
    return false;

  auto *Root = dyn_cast_or_null<yaml::MappingNode>(DocIt->getRoot());
  if (!Root)
    return false;

  for (auto &KV : *Root) {
    SmallString<64> KeyStorage;
    auto *KeyNode = dyn_cast<yaml::ScalarNode>(KV.getKey());
    if (!KeyNode)
      continue;
    StringRef Key = KeyNode->getValue(KeyStorage);

    if (Key == "protections") {
      auto *Map = dyn_cast<yaml::MappingNode>(KV.getValue());
      if (!Map)
        continue;
      for (auto &PKV : *Map) {
        SmallString<64> PKeyStorage;
        auto *PKey = dyn_cast<yaml::ScalarNode>(PKV.getKey());
        if (!PKey)
          continue;
        StringRef PName = PKey->getValue(PKeyStorage);

        bool Val = getYamlBool(PKV.getValue());
        if (PName == "string_encryption")
          Out.StringEncryption = Val;
        else if (PName == "control_flow_flattening")
          Out.ControlFlowFlattening = Val;
        else if (PName == "opaque_constants")
          Out.OpaqueConstants = Val;
        else if (PName == "arithmetic")
          Out.Arithmetic = Val;
        else if (PName == "anti_hook")
          Out.AntiHook = Val;
        else if (PName == "indirect_branch")
          Out.IndirectBranch = Val;
        else if (PName == "indirect_call")
          Out.IndirectCall = Val;
        else if (PName == "break_control_flow")
          Out.BreakControlFlow = Val;
        else if (PName == "basic_block_duplicate")
          Out.BasicBlockDuplicate = Val;
        else if (PName == "function_outline")
          Out.FunctionOutline = Val;
        else if (PName == "opaque_field_access")
          Out.OpaqueFieldAccess = Val;
        else
          SWARN("Unknown protection: {}", PName.str());
      }
    } else if (Key == "exclude") {
      Out.Exclude = getYamlStringList(KV.getValue());
    } else if (Key == "include") {
      Out.Include = getYamlStringList(KV.getValue());
    } else if (Key == "probability") {
      Out.Probability = getYamlInt(KV.getValue());
    } else if (Key == "license_key") {
      Out.LicenseKey = getYamlString(KV.getValue());
    }
  }

  SINFO("YAML config loaded: string_enc={}, cff={}, opaque={}, arith={}, "
        "anti_hook={}, indirect_br={}, indirect_call={}, break_cfg={}, "
        "bb_dup={}, fn_outline={}, opaque_field={}",
        Out.StringEncryption, Out.ControlFlowFlattening, Out.OpaqueConstants,
        Out.Arithmetic, Out.AntiHook, Out.IndirectBranch, Out.IndirectCall,
        Out.BreakControlFlow, Out.BasicBlockDuplicate, Out.FunctionOutline,
        Out.OpaqueFieldAccess);

  return true;
}

} // end namespace chaos_android
