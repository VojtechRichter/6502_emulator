#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_MEM 1024 * 64

typedef unsigned char Byte;
typedef unsigned short Word;

typedef unsigned int u32;

typedef struct {
    Byte data[MAX_MEM];
} Memory;

Memory *mem_init()
{
    Memory *mem = (Memory *)malloc(sizeof(Memory));    

    for (u32 i = 0; i < MAX_MEM; i++) {
        mem->data[i] = 0;
    }

    return mem;
}

typedef struct {
    Word PC;
    Byte SP;

    Byte A, X, Y;

    Byte PS;
} CPU;

CPU *cpu_reset(Memory **mem)
{
    CPU *cpu = (CPU *)malloc(sizeof(CPU));
    cpu->PC = 0xFFFC;
    cpu->SP = 0x0100;

    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;

    cpu->PS &= 0;
    
    *mem = mem_init();

    return cpu;
}

Byte cpu_fetch_byte(CPU *cpu, Memory *mem, u32 *cycles)
{
    Byte data = mem->data[cpu->PC];
    cpu->PC++;
    *cycles -= 1;

    return data;
}

Byte cpu_read_byte(Byte addr, CPU *cpu, Memory* mem, u32 *cycles)
{
    Byte data = mem->data[addr];
    *cycles -= 1;

    return data;
}

#define INS_LDA_IM 0xA9
#define INS_LDA_ZP 0xA5
#define INS_LDA_ZP_X 0xB5

void lda_set_ps_flags(CPU *cpu)
{
    // Zero Flag (Z)
    if (cpu->A == 0) {
        cpu->PS |= 0b00000010;
    }

    // Negative Flag (N)
    if ((cpu->A & 0b10000000) > 0) {
        cpu->PS |= 0b10000000;
    }
}

void cpu_execute(CPU *cpu, Memory *mem, u32 *cycles)
{
    while (*cycles > 0) {
        Byte ins = cpu_fetch_byte(cpu, mem, cycles);
        switch (ins) {
            case INS_LDA_IM: {
                Byte value = cpu_fetch_byte(cpu, mem, cycles);
                cpu->A = value;

                lda_set_ps_flags(cpu);
            } break;

            case INS_LDA_ZP: {
                Byte zp_addr = cpu_fetch_byte(cpu, mem, cycles);
                cpu->A = cpu_read_byte(zp_addr, cpu, mem, cycles);

                lda_set_ps_flags(cpu);
            } break;

            case INS_LDA_ZP_X: {
                Byte zp_addr = cpu_fetch_byte(cpu, mem, cycles);
                // what if it overflows?
                zp_addr += cpu->X; // takes single cycle
                *cycles -= 1;

                cpu->A = cpu_read_byte(zp_addr, cpu, mem, cycles);
                lda_set_ps_flags(cpu);
            } break;

            // case INS_LDA_ZP_X: {
            //     Byte zp_addr
            // } break;

            default: {
                printf("Instruction not supported: %d\n", ins);
            } break;
        }

    }
}

void release_resources(Memory *mem, CPU *cpu)
{
    free(mem);
    free(cpu);
}

int main(void)
{
    Memory *mem = NULL;
    CPU *cpu = cpu_reset(&mem);

    mem->data[0xFFFC] = INS_LDA_ZP;
    mem->data[0xFFFC] = 0x69;
    mem->data[0x0069] = 0x84;

    u32 cycles = 3;
    cpu_execute(cpu, mem, &cycles);

    release_resources(mem, cpu);

    return 0;
}