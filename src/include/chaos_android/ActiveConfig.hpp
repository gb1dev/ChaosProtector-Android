#pragma once

//
// ChaosProtector Android - Active configuration accessor
//

namespace chaos_android {

struct ObfuscationConfig;

// Returns the currently active ObfuscationConfig:
// - YamlObfuscationConfig if using YAML-only mode
// - PyObfuscationConfig if using Python config mode
ObfuscationConfig *getActiveConfig();

} // end namespace chaos_android
