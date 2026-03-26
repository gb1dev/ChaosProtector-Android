#pragma once

//
// ChaosProtector Android - License validation
//

#include <string>
#include "chaos_android/YamlObfuscationConfig.hpp"

namespace chaos_android {

class LicenseValidator {
public:
  // Validate a license key and return the tier
  static LicenseTier validate(const std::string &Key);

  // Apply tier restrictions to the protection config
  // Disables protections not available in the given tier
  static void applyTierRestrictions(YamlProtectionConfig &Config,
                                    LicenseTier Tier);

  // Get human-readable tier name
  static const char *tierName(LicenseTier Tier);

private:
  // Verify key format: CP-XXXX-XXXX-XXXX-XXXX
  static bool isValidFormat(const std::string &Key);

  // Server-side activation (calls ChaosProtector API)
  static LicenseTier activateOnline(const std::string &Key);
};

} // end namespace chaos_android
