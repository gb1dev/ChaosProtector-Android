#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/Config/llvm-config.h"
#include "llvm/Support/VCSRevision.h"

#include "chaos_android/config.hpp"

#ifdef LLVM_REVISION
#define CHAOS_ANDROID_LLVM_VERSION        " (" LLVM_REVISION ")"
#else
#define CHAOS_ANDROID_LLVM_VERSION
#endif

#define CHAOS_ANDROID_LLVM_REPO           LLVM_REPOSITORY
#define CHAOS_ANDROID_LLVM_MAJOR          LLVM_VERSION_MAJOR
#define CHAOS_ANDROID_LLVM_MINOR          LLVM_VERSION_MINOR
#define CHAOS_ANDROID_LLVM_PATCH          LLVM_VERSION_PATCH
#define CHAOS_ANDROID_LLVM_VERSION_STRING LLVM_VERSION_STRING
