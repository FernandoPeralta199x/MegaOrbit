/* MegaOrbit v0.3.0-dev - hardware emulator diagnostics on Sega Saturn.
 * NOT compatible with commercial ROMs. Direct SCSP PCM diagnostic.
 */
#include <yaul.h>
#include <stdint.h>
#include <string.h>
#include "megacore.h"
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
 unsigned i,begin=0u,end,n;
 dbgio_printf("\033[H\033[2J");
 dbgio_printf("MEGA ORBIT V0.4.0-DEV\n");
 dbgio_printf("GENESIS / MEGA DRIVE EMULATOR\n");
 dbgio_printf("VERSION 1.0.0 [MENU]\n");
 dbgio_printf("GRIFFONBR\n");
 dbgio_printf("----------------------------\n");
 dbgio_printf("MENU E DIAGNOSTICO\n\n");
 dbgio_printf("[%s]\n",ui_page_name(ui.page));
 if(ui.page==UI_OPTIONS)dbgio_printf("ABA: %s   ESQ/DIR\n",ui_tab_label(ui.tab));
 dbgio_printf("\n");
 if(ui.page==UI_MAIN||ui.page==UI_ROMS||ui.page==UI_OPTIONS||ui.page==UI_PAUSE){
  n=ui_item_count(&ui);end=n;
  if(n>6u){
   begin=(ui.cursor>=5u)?ui.cursor-5u:0u;
   end=begin+6u;
   if(end>n)end=n;
  }
  for(i=begin;i<end;i++)
   dbgio_printf("%c %s\n",ui.cursor==i?'>':' ',ui_item_label(&ui,i));
  if(n>6u)dbgio_printf(" %u-%u / %u\n",begin+1u,end,n);
 }
 if(ui.page==UI_OPTIONS){
  if(ui.tab==0u){
   dbgio_printf("\nTILE/CRAM TESTE: %s\n",vdp_ok?"PASS":"FAIL");
   dbgio_printf("PIXEL HASH: %08lX\n",(unsigned long)vdp_sum);
   dbgio_printf("NAO RENDERIZA JOGOS AINDA\n");
  }else if(ui.tab==1u){
   dbgio_printf("\nA/C: TOCAR AUDIO TESTE\n");
   dbgio_printf("B: PARAR E VOLTAR\n");
   dbgio_printf("SCSP: %s\n",sound_active?"ATIVO (TESTE)":"PARADO");
  }else if(ui.tab==2u){
   dbgio_printf("\nZ80 TESTE: %s\n",z80_ok?"PASS":"FAIL");
   dbgio_printf("Z80 A=%02X PC=%04X\n",(unsigned)z80.a,(unsigned)z80.pc);
  }else dbgio_printf("\nCONTROLE SATURN: PORTA 1\n");
 }
 if(ui.page==UI_DIAGNOSTIC){
  dbgio_printf("STATUS 68000: %s\n",diagnostic_ok?"PASS":"FAIL");
  dbgio_printf("PASSOS %u/5 PC=%08lX\n",diagnostic_steps,(unsigned long)cpu.pc);
  dbgio_printf("D0 %08lX D1 %08lX\n",(unsigned long)cpu.d[0],(unsigned long)cpu.d[1]);
  dbgio_printf("L+R+START: MENU DE PAUSA\n");
 }
 if(ui.page==UI_PAUSE)dbgio_printf("\nSTATUS: %s\n",ui.status?ui.status:"");
 if(ui.page==UI_SAVES)dbgio_printf("SAVE STATES NAO DISPONIVEIS\n");
 if(ui.page==UI_CREDITS)dbgio_printf("LIBYAUL + PICODRIVE (REFERENCIA)\n\nNAO AFILIADO A SEGA.\n");
 if(ui.page==UI_ROMS){
  dbgio_printf("\nDIRETORIO: /ROMS/MEGA/\n");
  dbgio_printf("SONIC THE HEDGEHOG (USA/EUR)\n");
  dbgio_printf("512 KB - REGIAO JUE\n");
  dbgio_printf("OUTROS JOGOS: ARQUIVOS AUSENTES\n");
 }
 if(ui.page==UI_GAME_TEST){
  if(sonic_valid){
   dbgio_printf("SONIC (USA, EUROPE)\n");
   dbgio_printf("ROM: CARREGADA, 512 KB\n");
   dbgio_printf("REGIAO: JUE\n");
   dbgio_printf("PC RESET: %08lX\n",(unsigned long)sonic_start_pc);
   dbgio_printf("SP RESET: %08lX\n",(unsigned long)sonic_start_sp);
   dbgio_printf("PASSOS: %u / 512\n",sonic_steps);
   dbgio_printf("PC STOP: %08lX\n",(unsigned long)sonic_end_pc);
   dbgio_printf("OPCODE: %04X\n",(unsigned)sonic_last_opcode);
   dbgio_printf("STATUS: %s\n",mc_result_name(sonic_status));
   dbgio_printf("JOGO AINDA NAO JOGAVEL\n");
  }else{
   dbgio_printf("ROM NAO EMBUTIDA NESTE DISCO\n");
   dbgio_printf("USE PACOTE PRIVADO COM ROM\n");
  }
 }
 dbgio_printf("\n----------------------------\n");
 dbgio_printf("D-PAD NAVEGAR  A/C OK  B VOLTAR\n");
 dbgio_printf("VDP/Z80/FM AINDA INCOMPLETOS\n");
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
  if(ui.page==UI_DIAGNOSTIC&&(!ui.diagnostic_ran||last!=UI_DIAGNOSTIC))
   do_diagnostic();
  if(ui.page==UI_GAME_TEST&&last!=UI_GAME_TEST)
   do_sonic();
  if(sound_active&&ui.page!=UI_OPTIONS){megaorbit_scsp_stop();sound_active=0;}
  display();dbgio_flush();vdp2_sync();vdp2_sync_wait();
 }
}
