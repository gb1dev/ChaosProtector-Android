#pragma once

// Global protection enable flags.
// Set by plugin.cpp when YAML config is parsed.
// Checked by passes that need explicit enable (anti-debug, anti-root, anti-tamper, IR virtualization).

namespace chaos_android {
  extern bool g_EnableAntiDebug;
  extern bool g_EnableAntiRoot;
  extern bool g_EnableAntiTamper;
  extern bool g_EnableIRVirtualization;
}
