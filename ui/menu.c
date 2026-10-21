#include "menu.h"
#include <stdio.h>
#include <string.h>
static const char *const main_items[]={"SELECIONAR JOGO","OPCOES DE SISTEMA","GERENCIADOR DE SAVES","CREDITOS"};
static const char *const tabs[]={"VIDEO","AUDIO","HARDWARE","CONTROLES"};
static const char *const vid[]={"VDP MEGA: TILES (TESTE)","VDP SATURN: VDP2 (TEXTO)","VRAM 64K + CRAM + VSRAM"};
static const char *const aud[]={"PSG: TOM SINTETIZADO","YM2612: FM APROXIMADA","SCSP: TESTE PCM 16-BIT","A/C: TOCAR SOM (LOOP)"};
static const char *const hw[]={"68000: PARCIAL","Z80: 10 OPCODES TESTE","VDP: TILES E PALETAS","PicoDrive: SUBMODULO"};
static const char *const controls[]={"D-PAD -> D-PAD","A -> A","B -> B","C -> C","X -> X","Y -> Y","Z -> Z","START -> START","MENU: L + R + START"};
static const char *const pause[]={"CONTINUAR","SALVAR ESTADO","CARREGAR ESTADO","", "RESETAR TESTE","VOLTAR AO MENU"};
void ui_init(ui_state_t *u){memset(u,0,sizeof(*u));u->page=UI_MAIN;u->save_slot=1;u->status="PRONTO";}
const char *ui_page_name(ui_page_t p){
 switch(p){case UI_MAIN:return "MENU PRINCIPAL";case UI_ROMS:return "SELECIONAR JOGO";case UI_OPTIONS:return "OPCOES DE SISTEMA";case UI_SAVES:return "GERENCIADOR DE SAVES";case UI_CREDITS:return "CREDITOS";case UI_DIAGNOSTIC:return "DIAGNOSTICO 68000";case UI_PAUSE:return "JOGO PAUSADO";default:return "ERRO";}
}
const char *ui_tab_label(unsigned t){return tabs[t%4u];}
unsigned ui_item_count(const ui_state_t *u){
 if(u->page==UI_MAIN)return 4;
 if(u->page==UI_ROMS)return 1;
 if(u->page==UI_PAUSE)return 6;
 if(u->page==UI_OPTIONS)return u->tab==0?3u:(u->tab==1?4u:(u->tab==2?4u:9u));
 return 0;
}
const char *ui_item_label(const ui_state_t *u,unsigned n){
 static char slot[24];
 if(n>=ui_item_count(u))return "";
 if(u->page==UI_MAIN)return main_items[n];
 if(u->page==UI_ROMS)return "ROM SINTETICA 68000";
 if(u->page==UI_PAUSE){
  if(n==3){snprintf(slot,sizeof(slot),"SLOT [ %02u ]",u->save_slot);return slot;}
  return pause[n];
 }
 if(u->tab==0)return vid[n];
 if(u->tab==1)return aud[n];
 if(u->tab==2)return hw[n];
 return controls[n];
}
void ui_handle(ui_state_t *u,ui_input_t in){
 unsigned count=ui_item_count(u);
 if(in==UI_INPUT_PAUSE){
  if(u->page==UI_DIAGNOSTIC){u->paused_from=u->page;u->page=UI_PAUSE;u->cursor=0;u->status="MENU DE PAUSA";}
  else if(u->page==UI_PAUSE){u->page=u->paused_from;u->cursor=0;u->status="CONTINUAR";}
  return;
 }
 if(in==UI_INPUT_BACK){
  if(u->page==UI_PAUSE){u->page=u->paused_from;u->cursor=0;}
  else if(u->page==UI_DIAGNOSTIC){u->page=UI_ROMS;u->cursor=0;}
  else if(u->page!=UI_MAIN){u->page=UI_MAIN;u->cursor=u->last_main;}
  return;
 }
 if(in==UI_INPUT_LEFT||in==UI_INPUT_RIGHT){
  if(u->page==UI_OPTIONS){u->tab=(unsigned char)((u->tab+(in==UI_INPUT_LEFT?3u:1u))%4u);u->cursor=0;}
  else if(u->page==UI_PAUSE&&u->cursor==3u)
    u->save_slot=(unsigned char)(in==UI_INPUT_LEFT?(u->save_slot==1u?9u:u->save_slot-1u):(u->save_slot==9u?1u:u->save_slot+1u));
  return;
 }
 if(in==UI_INPUT_UP||in==UI_INPUT_DOWN){
  if(count)u->cursor=(unsigned char)((u->cursor+(in==UI_INPUT_UP?count-1u:1u))%count);
  return;
 }
 if(in!=UI_INPUT_ACCEPT)return;
 if(u->page==UI_MAIN){u->last_main=u->cursor;u->page=(ui_page_t)(u->cursor+1u);u->cursor=0;}
 else if(u->page==UI_ROMS){u->page=UI_DIAGNOSTIC;u->diagnostic_ran=0;}
 else if(u->page==UI_PAUSE){
  switch(u->cursor){
  case 0:u->page=u->paused_from;u->cursor=0;break;
  case 1:u->status="SAVE STATE NAO IMPLEMENTADO";break;
  case 2:u->status="LOAD STATE NAO IMPLEMENTADO";break;
  case 3:u->save_slot=(unsigned char)(u->save_slot%9u+1u);break;
  case 4:u->diagnostic_ran=0;u->page=u->paused_from;u->cursor=0;break;
  case 5:u->page=UI_MAIN;u->cursor=0;break;
  }
 }
}
