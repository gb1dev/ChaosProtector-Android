#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Bytecode header
typedef struct {
    uint32_t magic;
    uint16_t numLocals;
    uint16_t numArgs;
    uint32_t codeSize;
    uint32_t stackSize;
} BytecodeHeader;

// Reproduce the interpreter in C to debug
int64_t chaos_vm_debug(const uint8_t *bc, int64_t *args, int nargs) {
    BytecodeHeader hdr;
    memcpy(&hdr, bc, sizeof(hdr));
    printf("Header: magic=0x%08X locals=%d args=%d code=%d stack=%d\n",
           hdr.magic, hdr.numLocals, hdr.numArgs, hdr.codeSize, hdr.stackSize);

    int64_t locals[256] = {0};
    int64_t stack[64] = {0};
    int sp = 0;
    int ip = sizeof(BytecodeHeader);

    // Copy args
    for (int i = 0; i < nargs && i < hdr.numLocals; i++)
        locals[i] = args[i];

    int running = 1;
    int steps = 0;
    while (running && steps < 1000) {
        uint8_t op = bc[ip++];
        steps++;
        printf("  [%d] op=0x%02X sp=%d\n", ip-1, op, sp);

        switch (op) {
        case 0x01: { // PUSH_IMM32
            int32_t v; memcpy(&v, &bc[ip], 4); ip += 4;
            stack[sp++] = (int64_t)v;
            break;
        }
        case 0x10: { // LOAD_LOCAL
            uint16_t idx; memcpy(&idx, &bc[ip], 2); ip += 2;
            stack[sp++] = locals[idx];
            break;
        }
        case 0x11: { // STORE_LOCAL
            uint16_t idx; memcpy(&idx, &bc[ip], 2); ip += 2;
            locals[idx] = stack[--sp];
            break;
        }
        case 0xA0: { // LOAD_ARG
            uint8_t idx = bc[ip++];
            stack[sp++] = args[idx];
            break;
        }
        case 0x20: { // ADD
            int64_t b = stack[--sp];
            int64_t a = stack[--sp];
            stack[sp++] = a + b;
            break;
        }
        case 0x90: { // RET
            int64_t ret = stack[--sp];
            printf("  RET = %lld\n", (long long)ret);
            return ret;
        }
        case 0x91: running = 0; break; // RET_VOID
        default:
            printf("  UNKNOWN opcode 0x%02X\n", op);
            running = 0;
            break;
        }
    }
    return 0;
}

// The actual add bytecode will be linked by the obfuscator
extern const uint8_t __chaos_bc_add[];

int add_plain(int a, int b) { return a + b; }

int main() {
    // Test with plain C interpreter on the bytecode
    int64_t test_args[2] = {3, 7};
    extern int add(int, int);

    printf("Calling virtualized add(3, 7)...\n");
    int result = add(3, 7);
    printf("Result: %d (expected 10)\n", result);
    return result == 10 ? 0 : 1;
}
