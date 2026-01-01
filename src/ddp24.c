/*
 * DDP-24 Emulator
 * Viking Mars Lander Guidance Computer
 *
 * Based on DDP-24 Instruction Manual (August 1964)
 * Computer Control Company, Inc., Framingham, Massachusetts
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ddp24.h"

/* Sign-magnitude arithmetic helpers */
static inline int32_t to_signed(word_t w) {
    w &= WORD_MASK;
    if (w & SIGN_BIT) {
        return -(int32_t)(w & MAGNITUDE_MASK);
    }
    return (int32_t)(w & MAGNITUDE_MASK);
}

static inline word_t from_signed(int32_t v) {
    if (v < 0) {
        return SIGN_BIT | ((-v) & MAGNITUDE_MASK);
    }
    return v & MAGNITUDE_MASK;
}

/* Initialize CPU */
void ddp24_init(ddp24_t *cpu) {
    memset(cpu, 0, sizeof(ddp24_t));
    cpu->halted = false;
    cpu->overflow = false;
    cpu->interrupt_enabled = false;
    cpu->cycles = 0;
    /* X[0] is hardwired to 0 */
    cpu->X[0] = 0;
}

/* Reset CPU (preserves memory) */
void ddp24_reset(ddp24_t *cpu) {
    cpu->A = 0;
    cpu->B = 0;
    cpu->X[0] = 0;
    cpu->X[1] = 0;
    cpu->X[2] = 0;
    cpu->X[3] = 0;
    cpu->PC = 0;
    cpu->halted = false;
    cpu->overflow = false;
    cpu->interrupt_enabled = false;
    cpu->cycles = 0;
}

/* Memory read */
word_t ddp24_read(ddp24_t *cpu, word_t addr) {
    addr &= (MEM_SIZE - 1);
    return cpu->memory[addr] & WORD_MASK;
}

/* Memory write */
void ddp24_write(ddp24_t *cpu, word_t addr, word_t value) {
    addr &= (MEM_SIZE - 1);
    cpu->memory[addr] = value & WORD_MASK;
}

/* Calculate effective address */
static word_t effective_address(ddp24_t *cpu, word_t instr) {
    word_t addr = decode_address(instr);
    uint8_t idx = decode_index(instr);

    /* Add index register (X[0] is always 0) */
    if (idx > 0) {
        addr = (addr + cpu->X[idx]) & ADDR_MASK;
    }

    /* Handle indirect addressing */
    if (decode_indirect(instr)) {
        addr = ddp24_read(cpu, addr) & ADDR_MASK;
    }

    return addr;
}

/* Execute one instruction, return cycles used */
int ddp24_step(ddp24_t *cpu) {
    if (cpu->halted) {
        return 0;
    }

    word_t instr = ddp24_read(cpu, cpu->PC);
    cpu->PC = (cpu->PC + 1) & ADDR_MASK;

    uint8_t op = decode_opcode(instr);
    word_t ea = effective_address(cpu, instr);
    word_t operand;
    int32_t sa, sb, result;
    int cycles = 5;  /* Default cycle count */

    switch (op) {
        case OP_HLT:  /* Halt */
            cpu->halted = true;
            cpu->PC = (cpu->PC - 1) & ADDR_MASK;  /* Stay at HLT */
            cycles = 5;
            break;

        case OP_NOP:  /* No Operation */
            cycles = 5;
            break;

        case OP_LDA:  /* Load A */
            cpu->A = ddp24_read(cpu, ea);
            cycles = 10;
            break;

        case OP_LDB:  /* Load B */
            cpu->B = ddp24_read(cpu, ea);
            cycles = 10;
            break;

        case OP_STA:  /* Store A */
            ddp24_write(cpu, ea, cpu->A);
            cycles = 10;
            break;

        case OP_STB:  /* Store B */
            ddp24_write(cpu, ea, cpu->B);
            cycles = 10;
            break;

        case OP_ADD:  /* Add */
            operand = ddp24_read(cpu, ea);
            sa = to_signed(cpu->A);
            sb = to_signed(operand);
            result = sa + sb;
            /* Check overflow */
            if (result > 0x7FFFFF || result < -0x7FFFFF) {
                cpu->overflow = true;
            }
            cpu->A = from_signed(result);
            cycles = 10;
            break;

        case OP_SUB:  /* Subtract */
            operand = ddp24_read(cpu, ea);
            sa = to_signed(cpu->A);
            sb = to_signed(operand);
            result = sa - sb;
            if (result > 0x7FFFFF || result < -0x7FFFFF) {
                cpu->overflow = true;
            }
            cpu->A = from_signed(result);
            cycles = 10;
            break;

        case OP_MPY:  /* Multiply */
            {
                operand = ddp24_read(cpu, ea);
                /* Get magnitudes and signs */
                uint32_t b_mag = cpu->B & MAGNITUDE_MASK;
                uint32_t y_mag = operand & MAGNITUDE_MASK;
                bool b_neg = (cpu->B & SIGN_BIT) != 0;
                bool y_neg = (operand & SIGN_BIT) != 0;
                bool result_neg = b_neg ^ y_neg;

                /* 46-bit product (23-bit * 23-bit = 46-bit) */
                uint64_t product = (uint64_t)b_mag * (uint64_t)y_mag;

                /* Most significant 23 bits to A, least significant 23 to B */
                uint32_t a_mag = (product >> 23) & MAGNITUDE_MASK;
                uint32_t new_b_mag = product & MAGNITUDE_MASK;

                /* Set signs to algebraic sign of product */
                cpu->A = (result_neg && (a_mag || new_b_mag)) ? (SIGN_BIT | a_mag) : a_mag;
                cpu->B = (result_neg && (a_mag || new_b_mag)) ? (SIGN_BIT | new_b_mag) : new_b_mag;
            }
            cycles = 28;  /* 14 usec average */
            break;

        case OP_DIV:  /* Divide */
            {
                operand = ddp24_read(cpu, ea);
                uint32_t divisor_mag = operand & MAGNITUDE_MASK;
                uint32_t a_mag = cpu->A & MAGNITUDE_MASK;
                bool dividend_neg = (cpu->A & SIGN_BIT) != 0;
                bool divisor_neg = (operand & SIGN_BIT) != 0;

                /* Check for improper divide (A >= divisor) */
                if (a_mag >= divisor_mag) {
                    /* Set improper divide indicator and skip */
                    cpu->overflow = true;
                    cycles = 44;
                    break;
                }

                /* Form 46-bit dividend from A:B */
                uint64_t dividend = ((uint64_t)(cpu->A & MAGNITUDE_MASK) << 23) |
                                    (cpu->B & MAGNITUDE_MASK);

                /* Divide */
                uint32_t quotient = dividend / divisor_mag;
                uint32_t remainder = dividend % divisor_mag;

                /* Quotient sign is algebraic, remainder sign is dividend sign */
                bool quotient_neg = dividend_neg ^ divisor_neg;

                cpu->B = (quotient_neg && quotient) ? (SIGN_BIT | quotient) : quotient;
                cpu->A = (dividend_neg && remainder) ? (SIGN_BIT | remainder) : remainder;
            }
            cycles = 44;  /* 22 usec */
            break;

        case OP_ANA:  /* AND to A */
            operand = ddp24_read(cpu, ea);
            cpu->A = (cpu->A & operand) & WORD_MASK;
            cycles = 10;
            break;

        case OP_ORA:  /* OR to A */
            operand = ddp24_read(cpu, ea);
            cpu->A = (cpu->A | operand) & WORD_MASK;
            cycles = 10;
            break;

        case OP_ERA:  /* Exclusive OR to A */
            operand = ddp24_read(cpu, ea);
            cpu->A = (cpu->A ^ operand) & WORD_MASK;
            cycles = 10;
            break;

        case OP_JMP:  /* Unconditional Jump */
            cpu->PC = ea;
            cycles = 5;
            break;

        case OP_JPL:  /* Jump if A Plus */
            if (!(cpu->A & SIGN_BIT) && (cpu->A & MAGNITUDE_MASK) != 0) {
                cpu->PC = ea;
            }
            cycles = 6;
            break;

        case OP_JMI:  /* Jump if A Minus */
            if (cpu->A & SIGN_BIT) {
                cpu->PC = ea;
            }
            cycles = 6;
            break;

        case OP_JZE:  /* Jump if A Zero */
            if ((cpu->A & MAGNITUDE_MASK) == 0) {
                cpu->PC = ea;
            }
            cycles = 6;
            break;

        case OP_JNZ:  /* Jump if A Not Zero */
            if ((cpu->A & MAGNITUDE_MASK) != 0) {
                cpu->PC = ea;
            }
            cycles = 6;
            break;

        case OP_JSL:  /* Jump and Store Location */
            ddp24_write(cpu, ea, cpu->PC);
            cpu->PC = (ea + 1) & ADDR_MASK;
            cycles = 10;
            break;

        case OP_SKG:  /* Skip if A Greater */
            operand = ddp24_read(cpu, ea);
            if (to_signed(cpu->A) > to_signed(operand)) {
                cpu->PC = (cpu->PC + 1) & ADDR_MASK;
            }
            cycles = 10;
            break;

        case OP_SKN:  /* Skip if A Not Equal */
            operand = ddp24_read(cpu, ea);
            if (cpu->A != operand) {
                cpu->PC = (cpu->PC + 1) & ADDR_MASK;
            }
            cycles = 10;
            break;

        case OP_TAB:  /* Transfer A to B */
            cpu->B = cpu->A;
            cycles = 5;
            break;

        case OP_IAB:  /* Interchange A and B */
            operand = cpu->A;
            cpu->A = cpu->B;
            cpu->B = operand;
            cycles = 10;
            break;

        case OP_LDX:  /* Load Index */
            {
                uint8_t idx = decode_index(instr);
                if (idx > 0) {
                    cpu->X[idx] = ddp24_read(cpu, ea) & ADDR_MASK;
                }
            }
            cycles = 5;
            break;

        case OP_SIX:  /* Store Index */
            {
                uint8_t idx = decode_index(instr);
                ddp24_write(cpu, ea, cpu->X[idx]);
            }
            cycles = 10;
            break;

        case OP_ARS:  /* A Right Shift */
            {
                word_t count = ea & 0x1F;  /* 5-bit shift count */
                word_t sign = cpu->A & SIGN_BIT;
                cpu->A = sign | ((cpu->A & MAGNITUDE_MASK) >> count);
            }
            cycles = 5 + (ea & 0x1F);
            break;

        case OP_ALS:  /* A Left Shift */
            {
                word_t count = ea & 0x1F;
                word_t sign = cpu->A & SIGN_BIT;
                cpu->A = sign | (((cpu->A & MAGNITUDE_MASK) << count) & MAGNITUDE_MASK);
            }
            cycles = 5 + (ea & 0x1F);
            break;

        case OP_XEC:  /* Execute */
            /* Execute instruction at EA without changing PC */
            cpu->PC = (ea + 1) & ADDR_MASK;
            cycles = 5 + ddp24_step(cpu);
            /* Note: PC changes from executed instruction are kept */
            break;

        default:
            fprintf(stderr, "Unimplemented opcode: %02o at PC=%05o\n", op, cpu->PC - 1);
            cpu->halted = true;
            break;
    }

    cpu->cycles += cycles;
    return cycles;
}

/* Run until halt or max_cycles */
int ddp24_run(ddp24_t *cpu, int max_cycles) {
    int total = 0;
    while (!cpu->halted && (max_cycles <= 0 || total < max_cycles)) {
        total += ddp24_step(cpu);
    }
    return total;
}

/* Dump CPU state */
void ddp24_dump(ddp24_t *cpu) {
    printf("=== DDP-24 CPU State ===\n");
    printf("PC: %05o  A: %08o  B: %08o\n", cpu->PC, cpu->A, cpu->B);
    printf("X1: %05o  X2: %05o  X3: %05o\n", cpu->X[1], cpu->X[2], cpu->X[3]);
    printf("Flags: %s%s%s\n",
           cpu->overflow ? "OVF " : "",
           cpu->halted ? "HLT " : "",
           cpu->interrupt_enabled ? "INT " : "");
    printf("Cycles: %llu\n", (unsigned long long)cpu->cycles);
}

/* Load binary file into memory */
int ddp24_load(ddp24_t *cpu, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror(filename);
        return -1;
    }

    /* Simple format: 3 bytes per word, big-endian */
    word_t addr = 0;
    uint8_t buf[3];
    while (fread(buf, 1, 3, f) == 3 && addr < MEM_SIZE) {
        cpu->memory[addr++] = (buf[0] << 16) | (buf[1] << 8) | buf[2];
    }

    fclose(f);
    printf("Loaded %d words from %s\n", addr, filename);
    return addr;
}
