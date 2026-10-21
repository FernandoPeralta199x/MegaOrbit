#ifndef MEGAORBIT_MENU_H
#define MEGAORBIT_MENU_H
typedef enum {
    UI_MAIN=0, UI_ROMS, UI_OPTIONS, UI_SAVES, UI_CREDITS, UI_DIAGNOSTIC
} ui_page_t;
typedef enum {
    UI_INPUT_UP=0, UI_INPUT_DOWN, UI_INPUT_LEFT, UI_INPUT_RIGHT,
    UI_INPUT_ACCEPT, UI_INPUT_BACK
} ui_input_t;
typedef struct {
    ui_page_t page;
    unsigned char cursor;
    unsigned char tab;
    unsigned char last_main;
    unsigned char last_options_tab;
    unsigned char diagnostic_ran;
} ui_state_t;
void ui_init(ui_state_t *ui);
void ui_handle(ui_state_t *ui, ui_input_t input);
const char *ui_page_name(ui_page_t page);
unsigned int ui_item_count(const ui_state_t *ui);
const char *ui_item_label(const ui_state_t *ui, unsigned int index);
const char *ui_tab_label(unsigned int tab);
#endif
