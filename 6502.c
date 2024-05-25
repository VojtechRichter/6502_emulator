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

Word cpu_fetch_word(CPU *cpu, Memory *mem, u32 *cycles)
{
    Word data = mem->data[cpu->PC];
    cpu->PC += 1;

    // little endian
    data |= (mem->data[cpu->PC] << 8);
    cpu->PC += 1;

    *cycles -= 2;

    return data;
}

Byte cpu_read_byte(Byte addr, CPU *cpu, Memory* mem, u32 *cycles)
{
    Byte data = mem->data[addr];
    *cycles -= 1;

    return data;
}

void write_word(Memory *mem, Word value, u32 addr, u32 *cycles)
{
    mem->data[addr] = value & 0xFF;
    mem->data[addr + 1] = (value >> 8);

    *cycles -= 2;
}

#define INS_LDA_IM 0xA9
#define INS_LDA_ZP 0xA5
#define INS_LDA_ZP_X 0xB5
#define INS_JSR 0x20

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
                // what if the address overflows?
                zp_addr += cpu->X; // takes single cycle
                *cycles -= 1;

                cpu->A = cpu_read_byte(zp_addr, cpu, mem, cycles);
                lda_set_ps_flags(cpu);
            } break;

            case INS_JSR: {
                Word sub_addr = cpu_fetch_word(cpu, mem, cycles);
                write_word(mem, cpu->PC - 1, cpu->SP, cycles);

                cpu->PC = sub_addr;

                *cycles -= 1;
            } break;

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

    mem->data[0xFFFC] = INS_JSR;
    mem->data[0xFFFD] = 0x42;
    mem->data[0xFFFE] = 0x42;
    mem->data[0x4242] = INS_LDA_IM;
    mem->data[0x4243] = 0x84;

    u32 cycles = 8;
    cpu_execute(cpu, mem, &cycles);

    release_resources(mem, cpu);

    return 0;
}