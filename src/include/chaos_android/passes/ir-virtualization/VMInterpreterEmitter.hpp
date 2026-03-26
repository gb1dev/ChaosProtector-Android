#pragma once

//
// ChaosProtector Android - VM Interpreter Emitter
//

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"

namespace chaos_android {
namespace vm {

// Emit the VM interpreter function into the given LLVM Module.
// Returns the created __chaos_vm_interpret function.
llvm::Function *emitVMInterpreter(llvm::Module &M);

} // end namespace vm
} // end namespace chaos_android
