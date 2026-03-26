#pragma once

//
// ChaosProtector Android - VM Opcode Definitions
//
// Stack-based virtual machine for executing virtualized LLVM IR.
// Each instruction is a variable-length bytecode:
//   [opcode:8] [operand1:varies] [operand2:varies]
//
// The VM operates on a 64-bit value stack. All values are widened to i64.
// Pointers are stored as i64 on the stack.
//

#include <cstdint>

namespace chaos_android {
namespace vm {

// Bytecode opcodes (8-bit)
enum Opcode : uint8_t {
  // Stack operations
  OP_NOP        = 0x00,
  OP_PUSH_IMM32 = 0x01, // Push 32-bit sign-extended immediate
  OP_PUSH_IMM64 = 0x02, // Push 64-bit immediate
  OP_POP        = 0x03, // Discard top of stack
  OP_DUP        = 0x04, // Duplicate top of stack
  OP_SWAP       = 0x05, // Swap top two values

  // Local variable access (index into locals array)
  OP_LOAD_LOCAL  = 0x10, // Push locals[imm16] onto stack
  OP_STORE_LOCAL = 0x11, // Pop stack into locals[imm16]

  // Arithmetic (pop 2, push 1)
  OP_ADD   = 0x20,
  OP_SUB   = 0x21,
  OP_MUL   = 0x22,
  OP_SDIV  = 0x23,
  OP_UDIV  = 0x24,
  OP_SREM  = 0x25,
  OP_UREM  = 0x26,
  OP_NEG   = 0x27, // Negate (pop 1, push 1)

  // Bitwise (pop 2, push 1)
  OP_AND   = 0x30,
  OP_OR    = 0x31,
  OP_XOR   = 0x32,
  OP_SHL   = 0x33,
  OP_LSHR  = 0x34, // Logical shift right
  OP_ASHR  = 0x35, // Arithmetic shift right
  OP_NOT   = 0x36, // Bitwise NOT (pop 1, push 1)

  // Comparison (pop 2, push 1 bool as i64)
  OP_CMP_EQ  = 0x40,
  OP_CMP_NE  = 0x41,
  OP_CMP_SLT = 0x42, // Signed less than
  OP_CMP_SLE = 0x43,
  OP_CMP_SGT = 0x44,
  OP_CMP_SGE = 0x45,
  OP_CMP_ULT = 0x46, // Unsigned less than
  OP_CMP_ULE = 0x47,
  OP_CMP_UGT = 0x48,
  OP_CMP_UGE = 0x49,

  // Memory (uses native pointers on stack)
  OP_LOAD8   = 0x50, // Pop ptr, push *(uint8_t*)ptr
  OP_LOAD16  = 0x51,
  OP_LOAD32  = 0x52,
  OP_LOAD64  = 0x53,
  OP_STORE8  = 0x54, // Pop value, pop ptr, *(uint8_t*)ptr = value
  OP_STORE16 = 0x55,
  OP_STORE32 = 0x56,
  OP_STORE64 = 0x57,

  // Type conversions
  OP_SEXT8  = 0x60, // Sign-extend from 8-bit
  OP_SEXT16 = 0x61,
  OP_SEXT32 = 0x62,
  OP_ZEXT8  = 0x63, // Zero-extend from 8-bit (mask)
  OP_ZEXT16 = 0x64,
  OP_ZEXT32 = 0x65,
  OP_TRUNC8  = 0x66,
  OP_TRUNC16 = 0x67,
  OP_TRUNC32 = 0x68,

  // Control flow
  OP_BR       = 0x70, // Unconditional branch: imm32 = bytecode offset
  OP_BR_COND  = 0x71, // Conditional: pop cond, if true jump to imm32
  OP_BR_FALSE = 0x72, // Pop cond, if false jump to imm32

  // Function calls
  OP_CALL_NATIVE = 0x80, // Pop nargs, pop func_ptr, pop args..., push result
  OP_CALL_INDIRECT = 0x81, // Same but via function pointer on stack

  // Return
  OP_RET      = 0x90, // Pop return value and return
  OP_RET_VOID = 0x91, // Return void

  // Arguments
  OP_LOAD_ARG = 0xA0, // Push function argument[imm8]

  // GEP (pointer arithmetic)
  OP_PTR_ADD = 0xB0, // Pop offset, pop ptr, push ptr+offset

  // PHI support (select)
  OP_SELECT = 0xC0, // Pop false_val, pop true_val, pop cond, push result

  // Floating point (64-bit double)
  OP_FADD  = 0xD0,
  OP_FSUB  = 0xD1,
  OP_FMUL  = 0xD2,
  OP_FDIV  = 0xD3,
  OP_FCMP_OEQ = 0xD4,
  OP_FCMP_OLT = 0xD5,
  OP_FCMP_OGT = 0xD6,
  OP_SITOFP   = 0xD7, // Signed int to double
  OP_FPTOSI   = 0xD8, // Double to signed int
  OP_UITOFP   = 0xD9,
  OP_FPTOUI   = 0xDA,

  // Special
  OP_HALT = 0xFF, // Stop execution (error)
};

// Bytecode header for a virtualized function
struct BytecodeHeader {
  uint32_t Magic;        // 'CVM\0' = 0x004D5643
  uint16_t NumLocals;    // Number of local variables
  uint16_t NumArgs;      // Number of function arguments
  uint32_t CodeSize;     // Size of bytecode in bytes
  uint32_t StackSize;    // Required stack depth
};

static constexpr uint32_t BYTECODE_MAGIC = 0x004D5643; // "CVM\0"

} // end namespace vm
} // end namespace chaos_android
