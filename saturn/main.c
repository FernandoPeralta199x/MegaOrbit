/* MegaOrbit v0.1.0-dev: Sega Saturn UI test; not a Mega Drive game runner.
 * Input follows yaul-org/libyaul-examples/dbgio-menu. */
#include <yaul.h>
#include <stdint.h>
#include "megacore.h"
#include "menu.h"

static smpc_peripheral_digital_t pad;
static ui_state_t ui;
static md_bus_t bus;
static m68k_t cpu;
static int diagnostic_ok;
static unsigned int diagnostic_steps;

/* 68000 reset SP FF0100, PC 8; NOP, MOVEQ +5, MOVEQ -1,
   BRA +2, skipped NOP, MOVEQ +7. */
static const uint8_t demo_rom[] = {
    0x00,0xFF,0x01,0x00, 0x00,0x00,0x00,0x08,
    0x4E,0x71, 0x70,0x05, 0x72,0xFF, 0x60,0x02,
    0x4E,0x71, 0x74,0x07
};

static void vblank_out_handler(void *work __unused)
{
    smpc_peripheral_intback_issue();
}

void user_init(void)
{
    smpc_peripheral_init();
    vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,
        VDP2_TVMD_HORZ_NORMAL_A, VDP2_TVMD_VERT_224);
    vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3, 0x01FFFE),
        RGB1555(1, 0, 1, 5));
    vdp_sync_vblank_out_set(vblank_out_handler, NULL);
    vdp2_tvmd_display_set();
}

static void do_diagnostic(void)
{
    unsigned int i;
    mc_result_t r;
    md_bus_init(&bus, demo_rom, sizeof(demo_rom));
    r = m68k_reset(&cpu, &bus);
    diagnostic_steps = 0u;
    for (i=0u; i<5u && r == MC_OK; ++i) {
        r = m68k_step(&cpu, &bus);
        if (r == MC_OK) ++diagnostic_steps;
    }
    diagnostic_ok=(r==MC_OK && diagnostic_steps==5u &&
        cpu.pc==0x14u && cpu.d[0]==5u &&
        cpu.d[1]==0xFFFFFFFFu && cpu.d[2]==7u);
    ui.diagnostic_ran=1u;
}

static void draw_menu(void)
{
    unsigned int i;
    dbgio_printf("\033[H\033[2J");
    dbgio_printf("*       MEGA  ORBIT       *\n");
    dbgio_printf("----------------------------\n");
    dbgio_printf("MEGA DRIVE NO SEGA SATURN\n");
    dbgio_printf("VERSAO 0.1.0-dev / UI TEST\n\n");
    dbgio_printf("[%s]\n\n", ui_page_name(ui.page));
    if (ui.page == UI_OPTIONS)
        dbgio_printf("ABA: %s (ESQ/DIR)\n\n", ui_tab_label(ui.tab));
    if (ui.page == UI_MAIN || ui.page == UI_ROMS || ui.page == UI_OPTIONS) {
        for (i=0u; i<ui_item_count(&ui); ++i)
            dbgio_printf("%c %s\n", ui.cursor==i ? '>' : ' ', ui_item_label(&ui,i));
    }
    if (ui.page == UI_ROMS) {
        dbgio_printf("\nDEMO EMBUTIDA - SEM JOGOS\n");
        dbgio_printf("A/C: EXECUTAR TESTE 68000\n");
        dbgio_printf("SD/CD E ROMS: PENDENTES\n");
    } else if (ui.page == UI_OPTIONS) {
        dbgio_printf("\nAPENAS CONSULTA. SEM BACKEND.\n");
    } else if (ui.page == UI_SAVES) {
        dbgio_printf("SRAM E SAVE STATES: PENDENTES\n");
        dbgio_printf("NENHUM SAVE FOI MODIFICADO.\n");
    } else if (ui.page == UI_CREDITS) {
        dbgio_printf("MEGAORBIT - PROJETO HOMEBREW\n");
        dbgio_printf("SDK: LIBYAUL / YAUL-ORG\n");
        dbgio_printf("NAO AFILIADO A SEGA.\n");
    } else if (ui.page == UI_DIAGNOSTIC) {
        dbgio_printf("ROM: TESTE SINTETICO 68000\n");
        dbgio_printf("STATUS: %s\n", diagnostic_ok ? "PASS" : "FAIL");
        dbgio_printf("PASSOS: %u/5\n", diagnostic_steps);
        dbgio_printf("PC: %08lX\n", (unsigned long)cpu.pc);
        dbgio_printf("D0: %08lX\n", (unsigned long)cpu.d[0]);
        dbgio_printf("D1: %08lX\n", (unsigned long)cpu.d[1]);
        dbgio_printf("D2: %08lX\n", (unsigned long)cpu.d[2]);
        dbgio_printf("ERRO: %s\n", mc_result_name(cpu.error));
    }
    dbgio_printf("\n----------------------------\n");
    dbgio_printf("D-PAD MOVER   A/C OK   B VOLTAR\n");
    dbgio_printf("VDP/Z80/AUDIO: NAO EMULADOS\n");
}

int main(void)
{
    dbgio_init();
    dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);
    dbgio_dev_font_load();
    ui_init(&ui);
    for (;;) {
        ui_page_t previous_page;
        smpc_peripheral_process();
        smpc_peripheral_digital_port(1, &pad);
        previous_page = ui.page;
        if (pad.pressed.button.up) ui_handle(&ui, UI_INPUT_UP);
        else if (pad.pressed.button.down) ui_handle(&ui, UI_INPUT_DOWN);
        else if (pad.pressed.button.left) ui_handle(&ui, UI_INPUT_LEFT);
        else if (pad.pressed.button.right) ui_handle(&ui, UI_INPUT_RIGHT);
        else if (pad.pressed.button.b) ui_handle(&ui, UI_INPUT_BACK);
        else if (pad.pressed.button.a || pad.pressed.button.c)
            ui_handle(&ui, UI_INPUT_ACCEPT);
        if (previous_page != UI_DIAGNOSTIC &&
            ui.page == UI_DIAGNOSTIC && !ui.diagnostic_ran)
            do_diagnostic();
        draw_menu();
        dbgio_flush();
        vdp2_sync();
        vdp2_sync_wait();
    }
}
