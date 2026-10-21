/* Saturn hardware diagnostic, not a commercial-ROM runner. */
#include <yaul.h>
#include <stdint.h>
#include "megacore.h"

/* 68K reset vectors: initial SP FF0100, PC 000008.
   NOP; MOVEQ #5,D0; MOVEQ #-1,D1; BRA.s +2; skipped NOP; MOVEQ #7,D2 */
static const uint8_t demo_rom[] = {
    0x00,0xFF,0x01,0x00, 0x00,0x00,0x00,0x08,
    0x4E,0x71, 0x70,0x05, 0x72,0xFF, 0x60,0x02,
    0x4E,0x71, 0x74,0x07
};

/* 64 KB RAM must be static; never allocate this on the SH-2 stack. */
static md_bus_t md_bus;
static m68k_t md_cpu;

void user_init(void)
{
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,
        VDP2_TVMD_HORZ_NORMAL_A, VDP2_TVMD_VERT_224);
    vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE), RGB1555(1, 0, 0, 0));
    vdp2_tvmd_display_set();
}

int main(void)
{
    uint32_t start_pc, end_pc;
    int i;
    int passed = 0;
    mc_result_t status;

    dbgio_init();
    dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
    dbgio_dev_font_load();

    md_bus_init(&md_bus, demo_rom, sizeof(demo_rom));
    status = m68k_reset(&md_cpu, &md_bus);
    start_pc = md_cpu.pc;
    for (i = 0; i < 5 && status == MC_OK; i++) {
        status = m68k_step(&md_cpu, &md_bus);
    }
    end_pc = md_cpu.pc;
    if (status == MC_OK && md_cpu.d[0] == 5u &&
        md_cpu.d[1] == 0xFFFFFFFFu && md_cpu.d[2] == 7u &&
        md_cpu.pc == 0x14u) passed = 1;

    dbgio_printf("MEGAORBIT - DIAGNOSTICO\n\n");
    dbgio_printf("CORE 68000: %s\n", passed ? "PASS" : "FAIL");
    dbgio_printf("PC INICIAL: %08lX\n", (unsigned long)start_pc);
    dbgio_printf("PC FINAL:   %08lX\n", (unsigned long)end_pc);
    dbgio_printf("D0: %08lX\n", (unsigned long)md_cpu.d[0]);
    dbgio_printf("D1: %08lX\n", (unsigned long)md_cpu.d[1]);
    dbgio_printf("D2: %08lX\n", (unsigned long)md_cpu.d[2]);
    dbgio_printf("OPCODE: %04X\n", (unsigned)md_cpu.last_opcode);
    dbgio_printf("CICLOS: %lu\n", (unsigned long)md_cpu.cycles);
    dbgio_printf("STATUS: %s\n", mc_result_name(status));
    dbgio_printf("NOTA: VDP/Z80/AUDIO PENDENTES\n");

    while (1) {
        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();
    }
}
