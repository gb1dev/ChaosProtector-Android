//
// ChaosProtector Android - License validation
//

#include "chaos_android/Licensing.hpp"
#include "chaos_android/log.hpp"

#include <cstring>
#include <fstream>
#include <regex>

#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/utsname.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#endif

namespace chaos_android {

bool LicenseValidator::isValidFormat(const std::string &Key) {
  // Format: CP-XXXX-XXXX-XXXX-XXXX (alphanumeric groups)
  static std::regex Pattern(
      "^CP-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}$");
  return std::regex_match(Key, Pattern);
}

LicenseTier LicenseValidator::activateOnline(const std::string &Key) {
  // Try to read cached license from ~/.chaosprotector/license.dat
  std::string Home;
#if defined(__linux__) || defined(__APPLE__)
  if (const char *H = getenv("HOME"))
    Home = H;
#elif defined(_WIN32)
  if (const char *H = getenv("USERPROFILE"))
    Home = H;
#endif

  if (Home.empty()) {
    SWARN("Cannot determine home directory for license cache");
    return LicenseTier::Free;
  }

  std::string CacheDir = Home + "/.chaosprotector";
  std::string CacheFile = CacheDir + "/android_license.dat";

  // Try reading cached tier
  {
    std::ifstream In(CacheFile);
    if (In.good()) {
      std::string CachedKey;
      int CachedTier = 0;
      if (std::getline(In, CachedKey) && In >> CachedTier) {
        if (CachedKey == Key) {
          auto Tier = static_cast<LicenseTier>(CachedTier);
          SINFO("Using cached license: {} ({})", Key, tierName(Tier));
          return Tier;
        }
      }
    }
  }

  // Online activation via curl to ChaosProtector API
  SINFO("Activating license online: {}", Key);

  std::string Cmd =
      "curl -s -m 10 -X POST https://chaosprotector.com/api/activate "
      "-H 'Content-Type: application/json' "
      "-d '{\"key\": \"" + Key + "\", \"product\": \"android\"}'";

  FILE *Pipe = popen(Cmd.c_str(), "r");
  if (!Pipe) {
    SWARN("Cannot contact license server, defaulting to Free tier");
    return LicenseTier::Free;
  }

  char Buffer[1024] = {};
  std::string Response;
  while (fgets(Buffer, sizeof(Buffer), Pipe)) {
    Response += Buffer;
  }
  int Status = pclose(Pipe);

  if (Status != 0 || Response.empty()) {
    SWARN("License activation failed, defaulting to Free tier");
    return LicenseTier::Free;
  }

  // Simple JSON parsing for {"tier": "basic"} or {"tier": "pro"}
  LicenseTier Result = LicenseTier::Free;
  if (Response.find("\"pro\"") != std::string::npos)
    Result = LicenseTier::Pro;
  else if (Response.find("\"basic\"") != std::string::npos)
    Result = LicenseTier::Basic;
  else if (Response.find("\"free\"") != std::string::npos)
    Result = LicenseTier::Free;

  // Cache the result
#if defined(__linux__) || defined(__APPLE__)
  std::string MkdirCmd = "mkdir -p " + CacheDir;
  system(MkdirCmd.c_str());
#elif defined(_WIN32)
  std::string MkdirCmd = "mkdir \"" + CacheDir + "\" 2>NUL";
  system(MkdirCmd.c_str());
#endif

  {
    std::ofstream Out(CacheFile);
    if (Out.good()) {
      Out << Key << "\n" << static_cast<int>(Result) << "\n";
    }
  }

  SINFO("License activated: {} ({})", Key, tierName(Result));
  return Result;
}

LicenseTier LicenseValidator::validate(const std::string &Key) {
  if (Key.empty()) {
    SINFO("No license key provided, using Free tier");
    return LicenseTier::Free;
  }

  // Development/testing key: CP-TEST-0000-0000-0000 gives Pro tier
  if (Key == "CP-TEST-0000-0000-0000") {
    SINFO("Development key detected, using Pro tier");
    return LicenseTier::Pro;
  }

  if (!isValidFormat(Key)) {
    SWARN("Invalid license key format: {}", Key);
    return LicenseTier::Free;
  }

  return activateOnline(Key);
}

void LicenseValidator::applyTierRestrictions(YamlProtectionConfig &Config,
                                             LicenseTier Tier) {
  // Free tier: only string encryption + anti-hook
  // Basic tier: + CFF, opaque constants, arithmetic, indirect, anti-debug
  // Pro tier: all features

  if (Tier < LicenseTier::Basic) {
    // Free tier restrictions
    if (Config.ControlFlowFlattening) {
      SWARN("Control Flow Flattening requires Basic tier or higher");
      Config.ControlFlowFlattening = false;
    }
    if (Config.OpaqueConstants) {
      SWARN("Opaque Constants requires Basic tier or higher");
      Config.OpaqueConstants = false;
    }
    if (Config.Arithmetic) {
      SWARN("MBA Arithmetic requires Basic tier or higher");
      Config.Arithmetic = false;
    }
    if (Config.IndirectBranch) {
      SWARN("Indirect Branches requires Basic tier or higher");
      Config.IndirectBranch = false;
    }
    if (Config.IndirectCall) {
      SWARN("Indirect Calls requires Basic tier or higher");
      Config.IndirectCall = false;
    }
    if (Config.BreakControlFlow) {
      SWARN("Break Control Flow requires Basic tier or higher");
      Config.BreakControlFlow = false;
    }
    if (Config.BasicBlockDuplicate) {
      SWARN("Basic Block Duplication requires Basic tier or higher");
      Config.BasicBlockDuplicate = false;
    }
    if (Config.FunctionOutline) {
      SWARN("Function Outlining requires Basic tier or higher");
      Config.FunctionOutline = false;
    }
    if (Config.OpaqueFieldAccess) {
      SWARN("Opaque Field Access requires Basic tier or higher");
      Config.OpaqueFieldAccess = false;
    }
  }

  // Pro-only features will be added in Phase 3 (IR Virtualization)
  // For now Basic and Pro have the same pass set

  SINFO("Tier {} applied: string_enc={}, cff={}, opaque={}, arith={}, "
        "anti_hook={}, indirect_br={}, indirect_call={}",
        tierName(Tier), Config.StringEncryption, Config.ControlFlowFlattening,
        Config.OpaqueConstants, Config.Arithmetic, Config.AntiHook,
        Config.IndirectBranch, Config.IndirectCall);
}

const char *LicenseValidator::tierName(LicenseTier Tier) {
  switch (Tier) {
  case LicenseTier::Free:
    return "Free";
  case LicenseTier::Basic:
    return "Basic";
  case LicenseTier::Pro:
    return "Pro";
  }
  return "Unknown";
}

} // end namespace chaos_android
