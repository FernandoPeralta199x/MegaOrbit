#ifndef MEGAORBIT_MENU_H
#define MEGAORBIT_MENU_H
typedef enum {
 UI_MAIN=0, UI_ROMS, UI_OPTIONS, UI_SAVES, UI_CREDITS, UI_DIAGNOSTIC, UI_PAUSE
} ui_page_t;
typedef enum {
 UI_INPUT_UP=0, UI_INPUT_DOWN, UI_INPUT_LEFT, UI_INPUT_RIGHT,
 UI_INPUT_ACCEPT, UI_INPUT_BACK, UI_INPUT_PAUSE
} ui_input_t;
typedef struct {
 ui_page_t page,paused_from;
 unsigned char cursor,tab,last_main,diagnostic_ran,save_slot;
 const char *status;
} ui_state_t;
void ui_init(ui_state_t *);
void ui_handle(ui_state_t *,ui_input_t);
const char *ui_page_name(ui_page_t);
unsigned ui_item_count(const ui_state_t *);
const char *ui_item_label(const ui_state_t *,unsigned);
const char *ui_tab_label(unsigned);
#endif
