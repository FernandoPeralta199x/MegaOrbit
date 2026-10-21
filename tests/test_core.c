#include "../core/megacore.h"
#include <stdio.h>
#include <string.h>

#define EXPECT(condition) do { \
    if (!(condition)) { printf("FAIL: %s (line %d)\n", #condition, __LINE__); return 0; } \
} while (0)

/* A small test ROM. Initial SP=0xFF0100, PC=8. */
static md_bus_t bus;
static m68k_t cpu;
static uint8_t rom[256];

static void word_at(size_t at, uint16_t word)
{
    rom[at] = (uint8_t)(word >> 8u);
    rom[at + 1u] = (uint8_t)word;
}

static void long_at(size_t at, uint32_t dword)
{
    word_at(at, (uint16_t)(dword >> 16u));
    word_at(at + 2u, (uint16_t)dword);
}

static int setup(void)
{
    memset(rom, 0, sizeof(rom));
    long_at(0, 0x00FF0100u);
    long_at(4, 8u);
    md_bus_init(&bus, rom, sizeof(rom));
    return m68k_reset(&cpu, &bus) == MC_OK;
}

static int test_nop(void)
{
    EXPECT(setup()); word_at(8, 0x4E71u);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK);
    EXPECT(cpu.pc == 10u && cpu.cycles == 4u);
    return 1;
}

static int test_moveq_plus(void)
{
    EXPECT(setup()); word_at(8, 0x7005u);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK);
    EXPECT(cpu.d[0] == 5u && !(cpu.sr & 0x0Fu));
    return 1;
}

static int test_moveq_minus(void)
{
    EXPECT(setup()); word_at(8, 0x72FFu);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK);
    EXPECT(cpu.d[1] == 0xFFFFFFFFu && (cpu.sr & 0x08u));
    return 1;
}

static int test_moveq_zero_and_x(void)
{
    EXPECT(setup()); word_at(8, 0x7400u);
    cpu.sr |= 0x001Fu;
    EXPECT(m68k_step(&cpu, &bus) == MC_OK);
    EXPECT(cpu.d[2] == 0u && (cpu.sr & 0x1Fu) == 0x14u);
    return 1;
}

static int test_bra_forward(void)
{
    EXPECT(setup()); word_at(8, 0x6002u); word_at(10, 0x4E71u); word_at(12, 0x7607u);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK && cpu.pc == 12u);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK && cpu.d[3] == 7u);
    return 1;
}

static int test_bra_backward(void)
{
    EXPECT(setup()); word_at(8, 0x60FEu);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK && cpu.pc == 8u);
    return 1;
}

static int test_bra_word(void)
{
    EXPECT(setup()); word_at(8, 0x6000u); word_at(10, 0x0004u);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK && cpu.pc == 16u);
    return 1;
}

static int test_bsr_rts(void)
{
    uint32_t pushed;
    EXPECT(setup()); word_at(8, 0x6104u); word_at(14, 0x4E75u);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK && cpu.pc == 14u);
    EXPECT(cpu.a[7] == 0xFF00FCu);
    EXPECT(md_read32(&bus, cpu.a[7], &pushed) == MC_OK && pushed == 10u);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK && cpu.pc == 10u);
    EXPECT(cpu.a[7] == 0xFF0100u);
    return 1;
}

static int test_jmp_absolute(void)
{
    EXPECT(setup()); word_at(8, 0x4EF9u); long_at(10, 0x00000020u);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK && cpu.pc == 32u);
    return 1;
}

static int test_jsr_rts(void)
{
    EXPECT(setup()); word_at(8, 0x4EB9u); long_at(10, 0x00000020u);
    word_at(32, 0x4E75u);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK && cpu.pc == 32u);
    EXPECT(m68k_step(&cpu, &bus) == MC_OK && cpu.pc == 14u);
    return 1;
}

static int test_memory_big_endian(void)
{
    uint32_t v;
    EXPECT(setup());
    EXPECT(md_write32(&bus, 0xFF1000u, 0x89ABCDEFu) == MC_OK);
    EXPECT(md_read32(&bus, 0xFF1000u, &v) == MC_OK && v == 0x89ABCDEFu);
    EXPECT(md_read32(&bus, 0x01FF1000u, &v) == MC_OK && v == 0x89ABCDEFu);
    EXPECT(md_read32(&bus, 0xFF1001u, &v) == MC_ERR_ADDRESS);
    EXPECT(md_write8(&bus, 0x200u, 0x55u) == MC_ERR_UNMAPPED);
    return 1;
}

static int test_unknown_op_halts(void)
{
    EXPECT(setup()); word_at(8, 0xFFFFu);
    EXPECT(m68k_step(&cpu, &bus) == MC_ERR_OPCODE);
    EXPECT(cpu.pc == 10u && cpu.last_opcode == 0xFFFFu);
    EXPECT(m68k_step(&cpu, &bus) == MC_ERR_HALTED);
    return 1;
}

int main(void)
{
    const struct { const char *name; int (*fn)(void); } tests[] = {
        {"NOP", test_nop}, {"MOVEQ positivo", test_moveq_plus},
        {"MOVEQ negativo", test_moveq_minus}, {"MOVEQ zero/X", test_moveq_zero_and_x},
        {"BRA positivo", test_bra_forward}, {"BRA negativo", test_bra_backward},
        {"BRA.w", test_bra_word}, {"BSR/RTS", test_bsr_rts},
        {"JMP absoluto", test_jmp_absolute}, {"JSR/RTS", test_jsr_rts},
        {"Big-endian e bus", test_memory_big_endian}, {"Opcode desconhecido", test_unknown_op_halts}
    };
    unsigned passed = 0;
    size_t i;
    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        int ok = tests[i].fn();
        printf("[%s] %s\n", ok ? "PASS" : "FAIL", tests[i].name);
        if (ok) passed++;
    }
    printf("RESULTADO: %u/%u testes aprovados\n", passed,
           (unsigned)(sizeof(tests) / sizeof(tests[0])));
    return (passed == sizeof(tests) / sizeof(tests[0])) ? 0 : 1;
}
