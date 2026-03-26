#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "chaos_android/passes/ObfuscationOpt.hpp"
#include "chaos_android/passes/anti-hook/AntiHook.hpp"
#include "chaos_android/passes/arithmetic/Arithmetic.hpp"
#include "chaos_android/passes/basic-block-duplicate/BasicBlockDuplicate.hpp"
#include "chaos_android/passes/break-cfg/BreakControlFlow.hpp"
#include "chaos_android/passes/cfg-flattening/ControlFlowFlattening.hpp"
#include "chaos_android/passes/cleaning/Cleaning.hpp"
#include "chaos_android/passes/function-outline/FunctionOutline.hpp"
#include "chaos_android/passes/indirect-branch/IndirectBranch.hpp"
#include "chaos_android/passes/indirect-call/IndirectCall.hpp"
#include "chaos_android/passes/logger-bind/LoggerBind.hpp"
#include "chaos_android/passes/objc-cleaner/ObjCleaner.hpp"
#include "chaos_android/passes/opaque-constants/OpaqueConstants.hpp"
#include "chaos_android/passes/opaque-field-access/OpaqueFieldAccess.hpp"
#include "chaos_android/passes/string-encoding/StringEncoding.hpp"
#include "chaos_android/passes/anti-debug/AntiDebug.hpp"
#include "chaos_android/passes/anti-root/AntiRoot.hpp"
#include "chaos_android/passes/anti-tamper/AntiTamper.hpp"
