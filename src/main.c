/*
 * DDP-24 Emulator - Main Entry Point
 * Viking Mars Lander Guidance Computer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ddp24.h"

static void print_usage(const char *prog) {
    printf("DDP-24 Emulator - Viking Mars Lander Guidance Computer\n\n");
    printf("Usage: %s [options] [program.bin]\n\n", prog);
    printf("Options:\n");
    printf("  -i        Interactive mode\n");
    printf("  -t        Run built-in tests\n");
    printf("  -d        Dump state after execution\n");
    printf("  -h        Show this help\n");
}

static void interactive_mode(ddp24_t *cpu) {
    char line[256];
    printf("DDP-24 Interactive Mode. Commands: s(tep), r(un), d(ump), m(emory), q(uit)\n");

    while (!cpu->halted) {
        printf("ddp24> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        switch (line[0]) {
            case 's':  /* Step */
                ddp24_step(cpu);
                printf("PC=%05o A=%08o B=%08o\n", cpu->PC, cpu->A, cpu->B);
                break;

            case 'r':  /* Run */
                ddp24_run(cpu, 0);
                printf("Halted after %llu cycles\n", (unsigned long long)cpu->cycles);
                break;

            case 'd':  /* Dump */
                ddp24_dump(cpu);
                break;

            case 'm':  /* Memory */
                {
                    unsigned int addr;
                    if (sscanf(line + 1, "%o", &addr) == 1) {
                        printf("[%05o] = %08o\n", addr, ddp24_read(cpu, addr));
                    } else {
                        printf("Usage: m <octal_addr>\n");
                    }
                }
                break;

            case 'q':  /* Quit */
                return;

            case '\n':
                break;

            default:
                printf("Unknown command. Use s, r, d, m <addr>, or q\n");
        }
    }

    if (cpu->halted) {
        printf("CPU halted.\n");
        ddp24_dump(cpu);
    }
}

static int run_tests(void) {
    ddp24_t cpu;
    int passed = 0;
    int failed = 0;

    printf("=== DDP-24 Instruction Tests ===\n\n");

    /* Test 1: LDA/STA */
    {
        ddp24_init(&cpu);
        cpu.memory[0] = (OP_LDA << OP_SHIFT) | 0x100;  /* LDA 100 */
        cpu.memory[1] = (OP_STA << OP_SHIFT) | 0x101;  /* STA 101 */
        cpu.memory[2] = (OP_HLT << OP_SHIFT);          /* HLT */
        cpu.memory[0x100] = 0x123456;

        ddp24_run(&cpu, 100);

        if (cpu.memory[0x101] == 0x123456) {
            printf("PASS: LDA/STA\n");
            passed++;
        } else {
            printf("FAIL: LDA/STA (got %06x, expected 123456)\n", cpu.memory[0x101]);
            failed++;
        }
    }

    /* Test 2: ADD */
    {
        ddp24_init(&cpu);
        cpu.memory[0] = (OP_LDA << OP_SHIFT) | 0x100;  /* LDA 100 */
        cpu.memory[1] = (OP_ADD << OP_SHIFT) | 0x101;  /* ADD 101 */
        cpu.memory[2] = (OP_STA << OP_SHIFT) | 0x102;  /* STA 102 */
        cpu.memory[3] = (OP_HLT << OP_SHIFT);          /* HLT */
        cpu.memory[0x100] = 0x000005;  /* 5 */
        cpu.memory[0x101] = 0x000003;  /* 3 */

        ddp24_run(&cpu, 100);

        if (cpu.memory[0x102] == 0x000008) {
            printf("PASS: ADD\n");
            passed++;
        } else {
            printf("FAIL: ADD (got %06x, expected 000008)\n", cpu.memory[0x102]);
            failed++;
        }
    }

    /* Test 3: SUB */
    {
        ddp24_init(&cpu);
        cpu.memory[0] = (OP_LDA << OP_SHIFT) | 0x100;  /* LDA 100 */
        cpu.memory[1] = (OP_SUB << OP_SHIFT) | 0x101;  /* SUB 101 */
        cpu.memory[2] = (OP_STA << OP_SHIFT) | 0x102;  /* STA 102 */
        cpu.memory[3] = (OP_HLT << OP_SHIFT);          /* HLT */
        cpu.memory[0x100] = 0x000008;  /* 8 */
        cpu.memory[0x101] = 0x000003;  /* 3 */

        ddp24_run(&cpu, 100);

        if (cpu.memory[0x102] == 0x000005) {
            printf("PASS: SUB\n");
            passed++;
        } else {
            printf("FAIL: SUB (got %06x, expected 000005)\n", cpu.memory[0x102]);
            failed++;
        }
    }

    /* Test 4: JMP */
    {
        ddp24_init(&cpu);
        cpu.memory[0] = (OP_JMP << OP_SHIFT) | 0x010;  /* JMP 010 */
        cpu.memory[1] = (OP_HLT << OP_SHIFT);          /* HLT (skipped) */
        cpu.memory[0x10] = (OP_LDA << OP_SHIFT) | 0x100;  /* LDA 100 */
        cpu.memory[0x11] = (OP_HLT << OP_SHIFT);          /* HLT */
        cpu.memory[0x100] = 0x424242;

        ddp24_run(&cpu, 100);

        if (cpu.A == 0x424242) {
            printf("PASS: JMP\n");
            passed++;
        } else {
            printf("FAIL: JMP (A=%06x, expected 424242)\n", cpu.A);
            failed++;
        }
    }

    /* Test 5: Conditional Jump (JZE) */
    {
        ddp24_init(&cpu);
        cpu.memory[0] = (OP_LDA << OP_SHIFT) | 0x100;  /* LDA 100 (load 0) */
        cpu.memory[1] = (OP_JZE << OP_SHIFT) | 0x010;  /* JZE 010 */
        cpu.memory[2] = (OP_LDA << OP_SHIFT) | 0x101;  /* LDA 101 (wrong path) */
        cpu.memory[3] = (OP_HLT << OP_SHIFT);
        cpu.memory[0x10] = (OP_LDA << OP_SHIFT) | 0x102;  /* LDA 102 (right path) */
        cpu.memory[0x11] = (OP_HLT << OP_SHIFT);
        cpu.memory[0x100] = 0x000000;  /* Zero */
        cpu.memory[0x101] = 0xBAD;
        cpu.memory[0x102] = 0x600D;

        ddp24_run(&cpu, 100);

        if (cpu.A == 0x600D) {
            printf("PASS: JZE\n");
            passed++;
        } else {
            printf("FAIL: JZE (A=%06x, expected 00600d)\n", cpu.A);
            failed++;
        }
    }

    /* Test 6: ANA (AND) */
    {
        ddp24_init(&cpu);
        cpu.memory[0] = (OP_LDA << OP_SHIFT) | 0x100;
        cpu.memory[1] = (OP_ANA << OP_SHIFT) | 0x101;
        cpu.memory[2] = (OP_HLT << OP_SHIFT);
        cpu.memory[0x100] = 0xFF00FF;
        cpu.memory[0x101] = 0x0F0F0F;

        ddp24_run(&cpu, 100);

        if (cpu.A == 0x0F000F) {
            printf("PASS: ANA\n");
            passed++;
        } else {
            printf("FAIL: ANA (A=%06x, expected 0f000f)\n", cpu.A);
            failed++;
        }
    }

    /* Test 7: MPY (Multiply) - 100 * 50 = 5000 */
    {
        ddp24_init(&cpu);
        cpu.memory[0] = (OP_LDB << OP_SHIFT) | 0x100;  /* Load B with 100 */
        cpu.memory[1] = (OP_MPY << OP_SHIFT) | 0x101;  /* Multiply by 50 */
        cpu.memory[2] = (OP_HLT << OP_SHIFT);
        cpu.memory[0x100] = 100;  /* B = 100 */
        cpu.memory[0x101] = 50;   /* Multiplier = 50 */

        ddp24_run(&cpu, 100);

        /* Result: 5000 should be in B (fits in lower 23 bits), A should be 0 */
        if (cpu.B == 5000 && cpu.A == 0) {
            printf("PASS: MPY\n");
            passed++;
        } else {
            printf("FAIL: MPY (A=%06x B=%06x, expected A=0 B=5000)\n", cpu.A, cpu.B);
            failed++;
        }
    }

    /* Test 8: MPY with sign - (-5) * 3 = -15 */
    {
        ddp24_init(&cpu);
        cpu.memory[0] = (OP_LDB << OP_SHIFT) | 0x100;  /* Load B with -5 */
        cpu.memory[1] = (OP_MPY << OP_SHIFT) | 0x101;  /* Multiply by 3 */
        cpu.memory[2] = (OP_HLT << OP_SHIFT);
        cpu.memory[0x100] = SIGN_BIT | 5;  /* B = -5 */
        cpu.memory[0x101] = 3;              /* Multiplier = 3 */

        ddp24_run(&cpu, 100);

        /* Result should be -15 in B */
        if (cpu.B == (SIGN_BIT | 15) && cpu.A == SIGN_BIT) {
            printf("PASS: MPY (signed)\n");
            passed++;
        } else {
            printf("FAIL: MPY signed (A=%06x B=%06x, expected A=%06x B=%06x)\n",
                   cpu.A, cpu.B, SIGN_BIT, SIGN_BIT | 15);
            failed++;
        }
    }

    /* Test 9: DIV - 5000 / 50 = 100 */
    {
        ddp24_init(&cpu);
        cpu.A = 0;       /* High part of dividend */
        cpu.B = 5000;    /* Low part of dividend */
        cpu.memory[0] = (OP_DIV << OP_SHIFT) | 0x100;  /* Divide by 50 */
        cpu.memory[1] = (OP_HLT << OP_SHIFT);
        cpu.memory[0x100] = 50;  /* Divisor */

        ddp24_run(&cpu, 100);

        /* Quotient in B, remainder in A */
        if (cpu.B == 100 && cpu.A == 0) {
            printf("PASS: DIV\n");
            passed++;
        } else {
            printf("FAIL: DIV (A=%06x B=%06x, expected A=0 B=100)\n", cpu.A, cpu.B);
            failed++;
        }
    }

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed;
}

int main(int argc, char *argv[]) {
    ddp24_t cpu;
    int interactive = 0;
    int dump = 0;
    int test = 0;
    const char *program = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0) {
            interactive = 1;
        } else if (strcmp(argv[i], "-d") == 0) {
            dump = 1;
        } else if (strcmp(argv[i], "-t") == 0) {
            test = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            program = argv[i];
        }
    }

    if (test) {
        return run_tests();
    }

    ddp24_init(&cpu);

    if (program) {
        if (ddp24_load(&cpu, program) < 0) {
            return 1;
        }
    }

    if (interactive) {
        interactive_mode(&cpu);
    } else if (program) {
        ddp24_run(&cpu, 0);
        if (dump) {
            ddp24_dump(&cpu);
        }
    } else {
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
