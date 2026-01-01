/*
 * DDP-24 Emulator
 * Viking Mars Lander Guidance Computer
 *
 * Based on DDP-24 Instruction Manual (August 1964)
 * Computer Control Company, Inc., Framingham, Massachusetts
 */

#ifndef DDP24_H
#define DDP24_H

#include <stdint.h>
#include <stdbool.h>

/* Word size: 24 bits */
typedef uint32_t word_t;  /* Using 32-bit for convenience, mask to 24 */

#define WORD_MASK       0x00FFFFFF  /* 24-bit mask */
#define SIGN_BIT        0x00800000  /* Bit 23 is sign */
#define MAGNITUDE_MASK  0x007FFFFF  /* Bits 0-22 are magnitude */

/* Instruction format:
 * Bits 23-18: Opcode (6 bits)
 * Bit 17:     Indirect flag (I)
 * Bits 16-15: Index register select (X)
 * Bits 14-0:  Address (15 bits)
 */
#define OP_SHIFT        18
#define OP_MASK         0x3F
#define INDIRECT_BIT    (1 << 17)
#define INDEX_SHIFT     15
#define INDEX_MASK      0x03
#define ADDR_MASK       0x7FFF

/* Memory size: 32K words (15-bit address) */
#define MEM_SIZE        32768

/* Opcodes (octal) */
typedef enum {
    OP_HLT = 000,   /* Halt */
    OP_XEC = 002,   /* Execute */
    OP_STB = 003,   /* Store B */
    OP_STC = 004,   /* Store Command Portion of A */
    OP_STA = 005,   /* Store A */
    OP_SAA = 006,   /* Store Address Portion of A */
    OP_INA = 007,   /* Input to Memory */
    OP_ADD = 010,   /* Add */
    OP_SUB = 011,   /* Subtract */
    OP_SKG = 012,   /* Skip if A Greater */
    OP_SKN = 013,   /* Skip if A Not Equal */
    OP_ANA = 015,   /* AND to A */
    OP_ORA = 016,   /* OR to A */
    OP_ERA = 017,   /* Exclusive OR to A */
    OP_ADM = 020,   /* Add Magnitude */
    OP_SBM = 021,   /* Subtract Magnitude */
    OP_LDB = 023,   /* Load B */
    OP_LDA = 024,   /* Load A */
    OP_EAB = 025,   /* Exchange A and B partial */
    OP_JSL = 027,   /* Jump and Store Location */
    OP_SMP = 030,   /* Step Multiple Precision */
    OP_FMB = 032,   /* Fill Memory Block */
    OP_DMB = 033,   /* Dump Memory Block */
    OP_MPY = 034,   /* Multiply */
    OP_DIV = 035,   /* Divide */
    OP_BCD = 036,   /* Binary to BCD Conversion */
    OP_DCB = 037,   /* BCD to Binary Conversion */
    OP_ARS = 040,   /* A Right Shift */
    OP_ALS = 041,   /* A Left Shift */
    OP_LRR = 042,   /* Long Right Rotate */
    OP_LLR = 043,   /* Long Left Rotate */
    OP_LRS = 044,   /* Long Right Shift */
    OP_LLS = 045,   /* Long Left Shift */
    OP_NRM = 046,   /* Normalize */
    OP_OCP = 050,   /* Output Control Pulse */
    OP_ITC = 051,   /* Interrupt Control */
    OP_ITA = 052,   /* Input to A */
    OP_OTA = 053,   /* Output from A */
    OP_SMX = 054,   /* Store and Modify Index */
    OP_TAB = 055,   /* Transfer A to B */
    OP_LDX = 056,   /* Load Index */
    OP_IAB = 057,   /* Interchange A and B */
    OP_SKS = 061,   /* Skip on Sense */
    OP_RND = 062,   /* Round */
    OP_TAX = 063,   /* Transfer A to Index */
    OP_SCR = 064,   /* Scale Right */
    OP_SCL = 065,   /* Scale Left */
    OP_SIX = 066,   /* Store Index */
    OP_RIX = 067,   /* Replace and Increment Index */
    OP_JPL = 070,   /* Jump if A Plus */
    OP_JZE = 071,   /* Jump if A Zero */
    OP_JMI = 072,   /* Jump if A Minus */
    OP_JNZ = 073,   /* Jump if A Not Zero */
    OP_JMP = 074,   /* Unconditional Jump */
    OP_JXI = 075,   /* Jump on Index Incremented */
    OP_NOP = 077,   /* No Operation */
} opcode_t;

/* CPU State */
typedef struct {
    word_t A;           /* Accumulator A */
    word_t B;           /* Accumulator B */
    word_t X[4];        /* Index registers (X0 is always 0) */
    word_t PC;          /* Program Counter */
    word_t memory[MEM_SIZE];

    /* Status flags */
    bool overflow;
    bool halted;
    bool interrupt_enabled;

    /* Cycle counter for timing */
    uint64_t cycles;
} ddp24_t;

/* Function prototypes */
void ddp24_init(ddp24_t *cpu);
void ddp24_reset(ddp24_t *cpu);
int ddp24_step(ddp24_t *cpu);
int ddp24_run(ddp24_t *cpu, int max_cycles);
void ddp24_dump(ddp24_t *cpu);

/* Memory operations */
word_t ddp24_read(ddp24_t *cpu, word_t addr);
void ddp24_write(ddp24_t *cpu, word_t addr, word_t value);
int ddp24_load(ddp24_t *cpu, const char *filename);

/* Instruction decode helpers */
static inline uint8_t decode_opcode(word_t instr) {
    return (instr >> OP_SHIFT) & OP_MASK;
}

static inline bool decode_indirect(word_t instr) {
    return (instr & INDIRECT_BIT) != 0;
}

static inline uint8_t decode_index(word_t instr) {
    return (instr >> INDEX_SHIFT) & INDEX_MASK;
}

static inline word_t decode_address(word_t instr) {
    return instr & ADDR_MASK;
}

#endif /* DDP24_H */
