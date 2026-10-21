#include "../ui/menu.h"
#include <stdio.h>
#include <string.h>
#define CK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
 ui_state_t u;
 unsigned i;
 ui_init(&u);
 CK(u.page==UI_MAIN);
 ui_handle(&u,UI_INPUT_ACCEPT);
 CK(u.page==UI_ROMS&&ui_item_count(&u)==3u);
 CK(strstr(ui_item_label(&u,0),"ORBIT PADDLE")!=NULL);
 ui_handle(&u,UI_INPUT_ACCEPT);
 CK(u.page==UI_HOMEBREW);
 ui_handle(&u,UI_INPUT_PAUSE);
 CK(u.page==UI_PAUSE&&ui_item_count(&u)==6u);
 for(i=0;i<3u;i++)ui_handle(&u,UI_INPUT_DOWN);
 CK(u.cursor==3u&&u.save_slot==1u);
 ui_handle(&u,UI_INPUT_RIGHT);
 CK(u.save_slot==2u);
 ui_handle(&u,UI_INPUT_LEFT);
 CK(u.save_slot==1u);
 ui_handle(&u,UI_INPUT_BACK);
 CK(u.page==UI_HOMEBREW);
 ui_handle(&u,UI_INPUT_PAUSE);
 for(i=0;i<5u;i++)ui_handle(&u,UI_INPUT_DOWN);
 ui_handle(&u,UI_INPUT_ACCEPT);
 CK(u.page==UI_MAIN);
 puts("PASS: homebrew menu entry, pause trigger, save slots and quit");
 return 0;
}
