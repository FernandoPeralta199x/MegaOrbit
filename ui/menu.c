#include "menu.h"
#include <string.h>

static const char *const main_items[4] = {
    "SELECIONAR JOGO", "OPCOES DE SISTEMA", "GERENCIADOR DE SAVES", "CREDITOS"
};
static const char *const tabs[4] = {"VIDEO", "AUDIO", "HARDWARE", "CONTROLES"};
static const char *const rom_items[1] = {"ROM SINTETICA - TESTE CPU"};
static const char *const video_items[3] = {
    "SAIDA SATURN: 320x224", "PROPORCAO: 4:3", "FILTROS: NAO DISPONIVEIS"
};
static const char *const audio_items[2] = {
    "YM2612: EM DESENVOLVIMENTO", "PSG: EM DESENVOLVIMENTO"
};
static const char *const hardware_items[3] = {
    "68000: PARCIAL", "Z80: EM DESENVOLVIMENTO", "VDP: EM DESENVOLVIMENTO"
};
static const char *const controls_items[3] = {
    "D-PAD: NAVEGAR", "A/C: CONFIRMAR", "B: VOLTAR"
};

void ui_init(ui_state_t *ui)
{
    memset(ui, 0, sizeof(*ui));
    ui->page = UI_MAIN;
}

const char *ui_page_name(ui_page_t page)
{
    switch (page) {
    case UI_MAIN: return "MENU PRINCIPAL";
    case UI_ROMS: return "SELECIONAR JOGO";
    case UI_OPTIONS: return "OPCOES DE SISTEMA";
    case UI_SAVES: return "GERENCIADOR DE SAVES";
    case UI_CREDITS: return "CREDITOS";
    case UI_DIAGNOSTIC: return "DIAGNOSTICO 68000";
    default: return "DESCONHECIDO";
    }
}
const char *ui_tab_label(unsigned int tab) { return tabs[tab % 4u]; }
unsigned int ui_item_count(const ui_state_t *ui)
{
    switch (ui->page) {
    case UI_MAIN: return 4u;
    case UI_ROMS: return 1u;
    case UI_OPTIONS:
        switch (ui->tab) {
        case 0: return 3u;
        case 1: return 2u;
        case 2: return 3u;
        default: return 3u;
        }
    default: return 0u;
    }
}
const char *ui_item_label(const ui_state_t *ui, unsigned int index)
{
    const char *const *items = 0;
    unsigned int size = ui_item_count(ui);
    if (index >= size) return "";
    switch (ui->page) {
    case UI_MAIN: items=main_items; break;
    case UI_ROMS: items=rom_items; break;
    case UI_OPTIONS:
        switch (ui->tab) {
        case 0: items=video_items; break;
        case 1: items=audio_items; break;
        case 2: items=hardware_items; break;
        default: items=controls_items; break;
        }
        break;
    default: return "";
    }
    return items[index];
}
void ui_handle(ui_state_t *ui, ui_input_t input)
{
    unsigned int count = ui_item_count(ui);
    if (input == UI_INPUT_BACK) {
        if (ui->page == UI_DIAGNOSTIC) {
            ui->page = UI_ROMS;
            ui->cursor = 0;
        } else if (ui->page != UI_MAIN) {
            ui->page = UI_MAIN;
            ui->cursor = ui->last_main;
        }
        return;
    }
    if (input == UI_INPUT_LEFT || input == UI_INPUT_RIGHT) {
        if (ui->page == UI_OPTIONS) {
            ui->tab = (unsigned char)((ui->tab + (input == UI_INPUT_LEFT ? 3u : 1u)) % 4u);
            ui->cursor = 0u;
        }
        return;
    }
    if (input == UI_INPUT_UP || input == UI_INPUT_DOWN) {
        if (count == 0u) return;
        if (input == UI_INPUT_UP)
            ui->cursor = (unsigned char)((ui->cursor + count - 1u) % count);
        else
            ui->cursor = (unsigned char)((ui->cursor + 1u) % count);
        return;
    }
    if (input != UI_INPUT_ACCEPT) return;
    if (ui->page == UI_MAIN) {
        ui->last_main = ui->cursor;
        ui->page = (ui_page_t)(ui->cursor + 1u);
        ui->cursor = 0u;
    } else if (ui->page == UI_ROMS) {
        ui->page = UI_DIAGNOSTIC;
        ui->diagnostic_ran = 0u;
    }
}
