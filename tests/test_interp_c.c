// Pure C implementation of the VM interpreter for debugging
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint32_t magic;
    uint16_t numLocals;
    uint16_t numArgs;
    uint32_t codeSize;
    uint32_t stackSize;
} BytecodeHeader;

int64_t vm_interpret(const uint8_t *bc, int64_t *args, int nargs) {
    BytecodeHeader hdr;
    memcpy(&hdr, bc, sizeof(hdr));

    int64_t locals[512];
    int64_t vstack[128];
    int sp = 0, ip = sizeof(BytecodeHeader);
    int64_t retval = 0;
    int running = 1;

    memset(locals, 0, sizeof(locals));
    for (int i = 0; i < nargs; i++) locals[i] = args[i];

    while (running) {
        uint8_t op = bc[ip++];
        switch (op) {
        case 0x01: { // PUSH_IMM32
            int32_t v; memcpy(&v, &bc[ip], 4); ip += 4;
            vstack[sp++] = (int64_t)v; break;
        }
        case 0x10: { // LOAD_LOCAL
            uint16_t idx; memcpy(&idx, &bc[ip], 2); ip += 2;
            vstack[sp++] = locals[idx]; break;
        }
        case 0x11: { // STORE_LOCAL
            uint16_t idx; memcpy(&idx, &bc[ip], 2); ip += 2;
            locals[idx] = vstack[--sp]; break;
        }
        case 0xA0: { // LOAD_ARG
            uint8_t idx = bc[ip++];
            vstack[sp++] = args[idx]; break;
        }
        case 0x20: { int64_t b=vstack[--sp], a=vstack[--sp]; vstack[sp++]=a+b; break; } // ADD
        case 0x21: { int64_t b=vstack[--sp], a=vstack[--sp]; vstack[sp++]=a-b; break; } // SUB
        case 0x22: { int64_t b=vstack[--sp], a=vstack[--sp]; vstack[sp++]=a*b; break; } // MUL
        case 0x40: { int64_t b=vstack[--sp], a=vstack[--sp]; vstack[sp++]=(a==b); break; } // CMP_EQ
        case 0x42: { int64_t b=vstack[--sp], a=vstack[--sp]; vstack[sp++]=(a<b); break; } // CMP_SLT
        case 0x45: { int64_t b=vstack[--sp], a=vstack[--sp]; vstack[sp++]=(a>=b); break; } // CMP_SGE
        case 0x70: { uint32_t t; memcpy(&t, &bc[ip], 4); ip = t; break; } // BR
        case 0x71: { // BR_COND
            uint32_t t; memcpy(&t, &bc[ip], 4); ip += 4;
            int64_t c = vstack[--sp];
            if (c) ip = t;
            break;
        }
        case 0x90: retval = vstack[--sp]; running = 0; break; // RET
        case 0x91: running = 0; break; // RET_VOID
        default:
            printf("UNKNOWN opcode 0x%02X at ip=%d\n", op, ip-1);
            running = 0; break;
        }
    }
    return retval;
}

// The add bytecode (from compiler output)
static const uint8_t add_bc[] = {
    'C','V','M',0, 3,0, 2,0, 12,0,0,0, 18,0,0,0,
    0xA0,0x00, 0xA0,0x01, 0x20, 0x11,0x02,0x00, 0x10,0x02,0x00, 0x90
};

int main() {
    int64_t args[2] = {3, 7};
    int64_t result = vm_interpret(add_bc, args, 2);
    printf("C interpreter: add(3,7) = %lld (expected 10)\n", (long long)result);

    args[0] = 100; args[1] = 200;
    result = vm_interpret(add_bc, args, 2);
    printf("C interpreter: add(100,200) = %lld (expected 300)\n", (long long)result);
    return 0;
}
