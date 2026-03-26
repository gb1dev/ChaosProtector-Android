#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "chaos_android/passes/anti-hook/AntiHookOpt.hpp"
#include "chaos_android/passes/arithmetic/ArithmeticOpt.hpp"
#include "chaos_android/passes/basic-block-duplicate/BasicBlockDuplicateOpt.hpp"
#include "chaos_android/passes/break-cfg/BreakControlFlowOpt.hpp"
#include "chaos_android/passes/cfg-flattening/ControlFlowFlatteningOpt.hpp"
#include "chaos_android/passes/function-outline/FunctionOutlineOpt.hpp"
#include "chaos_android/passes/indirect-branch/IndirectBranchOpt.hpp"
#include "chaos_android/passes/indirect-call/IndirectCallOpt.hpp"
#include "chaos_android/passes/opaque-constants/OpaqueConstantsOpt.hpp"
#include "chaos_android/passes/opaque-field-access/OpaqueFieldAccessOpt.hpp"
#include "chaos_android/passes/string-encoding/StringEncodingOpt.hpp"
