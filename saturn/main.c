/* MegaOrbit v0.3.0-dev - hardware emulator diagnostics on Sega Saturn.
 * NOT compatible with commercial ROMs. Direct SCSP PCM diagnostic.
 */
#include <yaul.h>
#include <stdint.h>
#include <string.h>
#include "megacore.h"
#include "homebrew.h"
#include "md_emulation.h"
#include "md_vdp.h"
#include "scsp_audio.h"
#include "menu.h"
/* Embeds a zero-content placeholder; private BIN patch replaces it locally.
 * The ROM itself is NEVER committed to the public GitHub repository. */
extern uint8_t asset_sonic_slot[];
#define SONIC_ROM_SIZE 524288u
#define SONIC_SLOT_OFFSET 32u
static unsigned sonic_valid,sonic_steps;
static uint32_t sonic_start_pc,sonic_end_pc,sonic_start_sp;
static uint16_t sonic_last_opcode;
static mc_result_t sonic_status;
/* Mini-game source code is original 68000 machine code in core/homebrew.c.
 * The Saturn supplies the UI and input/audio/video devices.
 * This is a proof of CPU-driven input, not an emulated Genesis VDP game. */
static uint8_t paddle_rom[ORBIT_CART_SIZE];
static unsigned game_ticks,game_score,game_target,game_collision_latch,game_fail;
static m68k_t quick_cpu;
static uint8_t quick_ram[MD_RAM_SIZE],quick_io[0x20],quick_tmss[4];
static unsigned quick_valid,quick_slot;
static ui_page_t quick_page;
static unsigned quick_score,quick_target;
static smpc_peripheral_digital_t pad;
static ui_state_t ui;
static md_bus_t bus;
static m68k_t cpu;
static md_z80_t z80;
static md_vdp_t vdp;
static uint16_t vdp_buffer[64u*48u];
static int diagnostic_ok,z80_ok,vdp_ok,sound_active;
static uint32_t vdp_sum;
static unsigned diagnostic_steps;
static const uint8_t demo_rom[]={
 0x00,0xff,0x01,0x00,0x00,0x00,0x00,0x08,
 0x4e,0x71,0x70,0x05,0x72,0xff,0x60,0x02,
 0x4e,0x71,0x74,0x07
};
static void vblank_out_handler(void *work __unused){smpc_peripheral_intback_issue();}
void user_init(void){
 smpc_peripheral_init();
 vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE,VDP2_TVMD_HORZ_NORMAL_A,VDP2_TVMD_VERT_224);
 vdp2_scrn_back_color_set(VDP2_VRAM_ADDR(3,0x01fffe),RGB1555(1,0,1,5));
 vdp_sync_vblank_out_set(vblank_out_handler,NULL);
 vdp2_tvmd_display_set();
}
static void do_diagnostic(void){
 unsigned i;mc_result_t r;
 md_bus_init(&bus,demo_rom,sizeof(demo_rom));
 r=m68k_reset(&cpu,&bus);diagnostic_steps=0;
 for(i=0;i<5u&&r==MC_OK;i++){
  r=m68k_step(&cpu,&bus);
  if(r==MC_OK)diagnostic_steps++;
 }
 diagnostic_ok=(r==MC_OK&&diagnostic_steps==5&&cpu.pc==0x14u&&
     cpu.d[0]==5u&&cpu.d[1]==0xffffffffu&&cpu.d[2]==7u);
 ui.diagnostic_ran=1;
}
static void do_sonic(void){
 const uint8_t *rom=asset_sonic_slot+SONIC_SLOT_OFFSET;
 unsigned i;
 sonic_valid=memcmp(rom+0x100u,"SEGA MEGA DRIVE",15u)==0;
 sonic_steps=0;
 sonic_start_pc=0;sonic_end_pc=0;sonic_start_sp=0;
 sonic_last_opcode=0;sonic_status=MC_ERR_UNMAPPED;
 if(!sonic_valid)return;
 md_bus_init(&bus,rom,SONIC_ROM_SIZE);
 sonic_status=m68k_reset(&cpu,&bus);
 sonic_start_pc=cpu.pc;
 sonic_start_sp=cpu.a[7];
 for(i=0u;i<512u&&sonic_status==MC_OK;i++){
  sonic_status=m68k_step(&cpu,&bus);
  if(sonic_status==MC_OK)sonic_steps++;
 }
 sonic_end_pc=cpu.pc;
 sonic_last_opcode=cpu.last_opcode;
 ui.diagnostic_ran=1u;
}
static void homebrew_start(void){
 orbit_cart_build(paddle_rom);
 md_bus_init(&bus,paddle_rom,ORBIT_CART_SIZE);
 game_fail=(m68k_reset(&cpu,&bus)!=MC_OK);
 game_ticks=0;game_score=0;game_target=14;game_collision_latch=0;
 ui.diagnostic_ran=1u;
}
static void homebrew_frame(void){
 unsigned i;unsigned position;
 if(game_fail)return;
 bus.io[3]=0x7fu;
 if(pad.held.button.left)bus.io[3]&=(uint8_t)~4u;
 if(pad.held.button.right)bus.io[3]&=(uint8_t)~8u;
 for(i=0;i<7u;i++){
  if(m68k_step(&cpu,&bus)!=MC_OK){game_fail=1u;return;}
 }
 game_ticks++;
 position=(cpu.d[1]&0xffffu)%26u;
 if(position==game_target){
  if(!game_collision_latch){game_score++;game_target=(game_target+7u)%26u;}
  game_collision_latch=1;
 }else game_collision_latch=0;
}
static void save_quick(void){
 quick_cpu=cpu;
 memcpy(quick_ram,bus.ram,MD_RAM_SIZE);
 memcpy(quick_io,bus.io,sizeof(bus.io));
 memcpy(quick_tmss,bus.tmss,sizeof(bus.tmss));
 quick_slot=ui.save_slot;
 quick_page=ui.paused_from;
 quick_score=game_score;quick_target=game_target;
 quick_valid=1u;
 ui.status="SALVO NA RAM (TEMPORARIO)";
}
static void load_quick(void){
 if(!quick_valid||quick_slot!=ui.save_slot||quick_page!=ui.paused_from){
  ui.status="SLOT SEM SAVE COMPATIVEL";
  return;
 }
 cpu=quick_cpu;
 memcpy(bus.ram,quick_ram,MD_RAM_SIZE);
 memcpy(bus.io,quick_io,sizeof(bus.io));
 memcpy(bus.tmss,quick_tmss,sizeof(bus.tmss));
 game_score=quick_score;game_target=quick_target;
 game_fail=0;
 ui.status="SAVE RESTAURADO DA RAM";
}
static void do_z80(void){
 unsigned i;
 md_z80_init(&z80);
 z80.ram[0]=0x3e;z80.ram[1]=0x5a; /* LD A,5A */
 z80.ram[2]=0xd3;z80.ram[3]=0x7f; /* OUT (7F),A */
 z80.ram[4]=0x76; /* HALT */
 for(i=0;i<3u;i++){if(md_z80_step(&z80)!=1)break;}
 z80_ok=(z80.halted&&z80.a==0x5au&&z80.last_port==0x7fu&&z80.last_data==0x5au);
}
static void do_vdp(void){
 unsigned i;
 md_vdp_demo(&vdp);
 md_vdp_render(&vdp,vdp_buffer,64u,64u,48u);
 vdp_sum=0;vdp_ok=0;
 for(i=0;i<64u*48u;i++){
  vdp_sum=vdp_sum*33u+vdp_buffer[i];
  if(vdp_buffer[i]!=vdp_buffer[0])vdp_ok=1;
 }
}
static void display(void){
 unsigned i,begin=0,end,n;
 dbgio_printf("\033[H\033[2J");
 dbgio_printf(" MEGA ORBIT  /  GRIFFONBR\n");
 dbgio_printf(" GENESIS / MEGA DRIVE\n");
 dbgio_printf(" v0.5 DEV  |  SAROO-STYLE\n");
 dbgio_printf("============================\n");
 if(ui.page==UI_MAIN){
  dbgio_printf("  BIBLIOTECA E SISTEMA\n\n");
  for(i=0;i<ui_item_count(&ui);i++)
   dbgio_printf(" %c %s\n",i==ui.cursor?'>':' ',ui_item_label(&ui,i));
  dbgio_printf("\n----------------------------\n");
  dbgio_printf("3 ITENS NO SELETOR DE JOGOS\n");
 }else if(ui.page==UI_ROMS){
  dbgio_printf(" /ROMS/MEGA/    [3 ITENS]\n");
  dbgio_printf("----------------------------\n");
  for(i=0;i<ui_item_count(&ui);i++)
   dbgio_printf(" %c %s\n",i==ui.cursor?'>':' ',ui_item_label(&ui,i));
  dbgio_printf("\n DETALHES:\n");
  if(ui.cursor==0)dbgio_printf("HOMEBREW | 68000 | CONTROLE\n");
  else if(ui.cursor==1)dbgio_printf("SONIC | 512 KB | USA/EUR\n");
  else dbgio_printf("OPCODES + RAM + CPU TESTE\n");
 }else if(ui.page==UI_OPTIONS){
  dbgio_printf(" CONFIGURACOES / %s\n",ui_tab_label(ui.tab));
  dbgio_printf("----------------------------\n");
  n=ui_item_count(&ui);begin=ui.cursor>5u?ui.cursor-5u:0u;
  end=(begin+6u)<n?begin+6u:n;
  for(i=begin;i<end;i++)
   dbgio_printf(" %c %s\n",ui.cursor==i?'>':' ',ui_item_label(&ui,i));
  dbgio_printf("ITEM %u/%u - ESQ/DIR ABA\n",ui.cursor+1u,n);
  if(ui.tab==0u)dbgio_printf("VDP TILES TEST: %s\n",vdp_ok?"PASS":"FAIL");
  if(ui.tab==1u)dbgio_printf("A TOCAR | SCSP: %s\n",sound_active?"ATIVO":"PARADO");
  if(ui.tab==2u)dbgio_printf("Z80 TEST: %s\n",z80_ok?"PASS":"FAIL");
 }else if(ui.page==UI_HOMEBREW){
  char bar[28];
  unsigned x=(cpu.d[1]&0xffffu)%26u;
  for(i=0;i<26u;i++)bar[i]='.';
  bar[game_target]='*';
  bar[x]='O';
  bar[26]='\0';
  dbgio_printf(" ORBIT PADDLE (68000)\n");
  dbgio_printf("----------------------------\n");
  dbgio_printf("ESQ / DIR: COLETE O ASTRO\n\n");
  dbgio_printf("|%s|\n",bar);
  dbgio_printf("PONTOS: %u   FRAMES: %u\n",game_score,game_ticks);
  dbgio_printf("CPU PC: %06lX\n",(unsigned long)cpu.pc);
  dbgio_printf("CPU D1: %04X\n",(unsigned)(cpu.d[1]&0xffffu));
  dbgio_printf("STATUS: %s\n",game_fail?"CPU PARADA":"EXECUTANDO");
  dbgio_printf("\nL+R+START: PAUSA\n");
 }else if(ui.page==UI_PAUSE){
  dbgio_printf("        JOGO PAUSADO\n");
  dbgio_printf("----------------------------\n");
  for(i=0;i<ui_item_count(&ui);i++)
   dbgio_printf(" %c %s\n",ui.cursor==i?'>':' ',ui_item_label(&ui,i));
  dbgio_printf("----------------------------\n");
  dbgio_printf(" %s\n",ui.status?ui.status:"");
  dbgio_printf("SAVE: RAM / 1 SLOT ATIVO\n");
  dbgio_printf("NAO GRAVA EM MEMORIA SATURN\n");
 }else if(ui.page==UI_GAME_TEST){
  dbgio_printf(" SONIC - DIAGNOSTICO\n");
  dbgio_printf("----------------------------\n");
  if(!sonic_valid)dbgio_printf("ROM AUSENTE NESTE DISCO\n");
  else {
   dbgio_printf("ROM 512 KB / JUE\n");
   dbgio_printf("PC INICIO: %08lX\n",(unsigned long)sonic_start_pc);
   dbgio_printf("PASSOS: %u / 512\n",sonic_steps);
   dbgio_printf("PC FIM: %08lX\n",(unsigned long)sonic_end_pc);
   dbgio_printf("OPCODE: %04X\n",(unsigned)sonic_last_opcode);
   dbgio_printf("ERRO: %s\n",mc_result_name(sonic_status));
   dbgio_printf("SONIC NAO ESTA JOGAVEL\n");
  }
 }else if(ui.page==UI_DIAGNOSTIC){
  dbgio_printf("TESTE 68000: %s\n",diagnostic_ok?"PASS":"FAIL");
  dbgio_printf("PASSOS: %u / 5\n",diagnostic_steps);
  dbgio_printf("PC: %08lX\n",(unsigned long)cpu.pc);
  dbgio_printf("D0: %08lX\n",(unsigned long)cpu.d[0]);
  dbgio_printf("L+R+START: PAUSA\n");
 }else if(ui.page==UI_SAVES){
  dbgio_printf("SAVE VOLATIL: RAM APENAS\n");
  dbgio_printf("NO JOGO: L+R+START\n");
  dbgio_printf("SALVAMENTO PERMANENTE: NAO\n");
 }else if(ui.page==UI_CREDITS){
  dbgio_printf("GRIFFONBR - MEGA ORBIT\n");
  dbgio_printf("LIBYAUL / PICODRIVE REF.\n");
  dbgio_printf("PROJETO NAO AFILIADO A SEGA\n");
 }
 dbgio_printf("\n============================\n");
 dbgio_printf("A/C OK   B VOLTAR  D-PAD\n");
}
int main(void){
 dbgio_init();dbgio_dev_default_init(DBGIO_DEV_VDP2_ASYNC);dbgio_dev_font_load();
 ui_init(&ui);do_z80();do_vdp();
 for(;;){
  ui_page_t last=ui.page;
  smpc_peripheral_process();smpc_peripheral_digital_port(1,&pad);
  if(pad.pressed.button.start&&pad.held.button.l&&pad.held.button.r)
    ui_handle(&ui,UI_INPUT_PAUSE);
  else if(pad.pressed.button.up)ui_handle(&ui,UI_INPUT_UP);
  else if(pad.pressed.button.down)ui_handle(&ui,UI_INPUT_DOWN);
  else if(pad.pressed.button.left)ui_handle(&ui,UI_INPUT_LEFT);
  else if(pad.pressed.button.right)ui_handle(&ui,UI_INPUT_RIGHT);
  else if(pad.pressed.button.b){
   if(sound_active){megaorbit_scsp_stop();sound_active=0;}
   ui_handle(&ui,UI_INPUT_BACK);
  }else if(pad.pressed.button.a||pad.pressed.button.c){
   if(ui.page==UI_OPTIONS&&ui.tab==1u){
    megaorbit_scsp_play_test();sound_active=1;
   }else if(ui.page==UI_OPTIONS&&ui.tab==2u){do_z80();}
   else if(ui.page==UI_OPTIONS&&ui.tab==0u){do_vdp();}
   else ui_handle(&ui,UI_INPUT_ACCEPT);
  }
  if(ui.page==UI_DIAGNOSTIC&&!ui.diagnostic_ran)
   do_diagnostic();
  if(ui.page==UI_GAME_TEST&&!ui.diagnostic_ran)
   do_sonic();
  if(ui.page==UI_HOMEBREW&&!ui.diagnostic_ran)homebrew_start();
  if(ui.page==UI_HOMEBREW&&last==UI_HOMEBREW)homebrew_frame();
  if(sound_active&&ui.page!=UI_OPTIONS){megaorbit_scsp_stop();sound_active=0;}
  display();dbgio_flush();vdp2_sync();vdp2_sync_wait();
 }
}
