/* Compact Sega Saturn VDP2 menu backdrop inspired by Saroo's 320x240 layout.
 * Renders RGB1555 palette-indexed 8-bit bitmap (512x256 backing, 320x224 visible).
 * NBG1 under DBGIO NBG3: VRAM bank 0 is reserved for this bitmap;
 * libyaul default DBGIO uses bank 3, leaving no VRAM bank collision.
 *
 * We deliberately generate the background procedurally: no HD images,
 * JPEG decode or texture expansion, fast startup and reliable cartridge size.
 */
#include <yaul.h>
#include <stdint.h>
#include "ui_background.h"
#define BG_WIDTH 512u
#define BG_VISIBLE 320u
#define BG_HEIGHT 224u
#define BG_PALETTE VDP2_CRAM_MODE_0_OFFSET(0, 0, 0)
#define BG_VRAM VDP2_VRAM_ADDR(0,0)
static uint8_t *const pixels=(uint8_t *)BG_VRAM;
static int last_dimmed=-1;
static uint16_t original_palette[26];
static uint16_t rgb(unsigned r,unsigned g,unsigned b){
 return RGB1555(1,r,g,b).raw;
}
static void pixel(unsigned x,unsigned y,uint8_t index){
 if(x<BG_VISIBLE&&y<BG_HEIGHT)pixels[y*BG_WIDTH+x]=index;
}
static void rect(unsigned x0,unsigned y0,unsigned x1,unsigned y1,uint8_t color){
 unsigned x,y;
 for(y=y0;y<y1 && y<BG_HEIGHT;y++)
 for(x=x0;x<x1 && x<BG_VISIBLE;x++)pixel(x,y,color);
}
static void outlines(unsigned x0,unsigned y0,unsigned x1,unsigned y1){
 rect(x0,y0,x1,y0+1u,20u);
 rect(x0,y1-1u,x1,y1,20u);
 rect(x0,y0,x0+1u,y1,20u);
 rect(x1-1u,y0,x1,y1,20u);
 rect(x0+2u,y0+2u,x1-2u,y0+3u,10u);
}
static uint8_t base_color(unsigned x,unsigned y){
 unsigned h;
 if(y<143u){
  h=(x*301u+y*1999u+(x*y*11u))&0xffffu;
  if((h&2047u)==34u)return 25u;
  if(y<45u)return 1u;
  if(y<100u)return 2u;
  return 3u;
 }
 if(y==143u||y==144u)return 18u;
 if(y>148u){
  unsigned d=y-143u;
  int dx=(int)x-160;
  unsigned adx=(unsigned)(dx<0?-dx:dx);
  if(((adx*64u)/(d+3u))%15u==0u)return 12u;
  if((d*d)/300u%16u==0u)return 11u;
  return 4u;
 }
 return 3u;
}
static void background(void){
 unsigned y,x;
 for(y=0;y<256u;y++)for(x=0;x<BG_WIDTH;x++){
  pixels[y*BG_WIDTH+x]=(y<BG_HEIGHT&&x<BG_VISIBLE)?
   base_color(x,y):0u;
 }
 /* Stable panel positions that allow NBG3 default 8x8 debug font. */
 rect(1u,0u,319u,30u,5u);
 rect(1u,31u,319u,33u,19u);
 rect(1u,36u,319u,167u,6u);
 outlines(1u,36u,319u,167u);
 rect(1u,193u,319u,224u,5u);
 outlines(1u,192u,319u,224u);
}
void megaorbit_ui_background_init(void){
 vdp2_scrn_bitmap_format_t format={
  .scroll_screen=VDP2_SCRN_NBG1,
  .ccc=VDP2_SCRN_CCC_PALETTE_256,
  .bitmap_size=VDP2_SCRN_BITMAP_SIZE_512X256,
  .palette_base=BG_PALETTE,
  .bitmap_base=BG_VRAM
 };
 volatile uint16_t *palette=(volatile uint16_t *)BG_PALETTE;
 vdp2_vram_cycp_t *cycle;
 unsigned i;
 for(i=0;i<256u;i++)palette[i]=rgb(0,0,0);
 palette[1]=rgb(0,0,6);
 palette[2]=rgb(0,0,9);
 palette[3]=rgb(0,0,12);
 palette[4]=rgb(1,0,7);
 palette[5]=rgb(0,2,11);
 palette[6]=rgb(0,0,7);
 palette[10]=rgb(0,9,20);
 palette[11]=rgb(6,2,15);
 palette[12]=rgb(11,1,24);
 palette[18]=rgb(2,15,30);
 palette[19]=rgb(0,11,24);
 palette[20]=rgb(4,23,31);
 palette[21]=rgb(0,10,19);
 palette[25]=rgb(23,23,31);
 for(i=0;i<26u;i++)original_palette[i]=palette[i];
 background();
 /* VRAM bank 0 for NBG1 bitmap, bank 3 remains DBGIO NBG3. */
 cycle=vdp2_vram_cycp_get();
 cycle->pt[0].t0=VDP2_VRAM_CYCP_CHPNDR_NBG1;
 cycle->pt[0].t1=VDP2_VRAM_CYCP_CHPNDR_NBG1;
 cycle->pt[0].t2=VDP2_VRAM_CYCP_CHPNDR_NBG1;
 cycle->pt[0].t3=VDP2_VRAM_CYCP_CHPNDR_NBG1;
 cycle->pt[0].t4=VDP2_VRAM_CYCP_NO_ACCESS;
 cycle->pt[0].t5=VDP2_VRAM_CYCP_NO_ACCESS;
 cycle->pt[0].t6=VDP2_VRAM_CYCP_NO_ACCESS;
 cycle->pt[0].t7=VDP2_VRAM_CYCP_NO_ACCESS;
 vdp2_vram_cycp_set(cycle);
 vdp2_scrn_bitmap_format_set(&format);
 vdp2_scrn_priority_set(VDP2_SCRN_NBG1,1);
 vdp2_scrn_display_set(vdp2_scrn_display_get()|VDP2_SCRN_DISP_NBG1);
}
void megaorbit_ui_background_selection(const ui_state_t *ui){
 unsigned i,index=ui->cursor;
 int dimmed=(ui->page==UI_PAUSE);
 volatile uint16_t *palette=(volatile uint16_t *)BG_PALETTE;
 if(dimmed!=last_dimmed) {
  /* Pause darkens the pixel-art layer; NBG3 foreground text remains legible.
     This is an overlay effect, not a capture of the emulated game framebuffer. */
  for(i=0;i<26u;i++){
   uint16_t raw=original_palette[i];
   if(dimmed){
    unsigned r=(raw>>10)&31u,g=(raw>>5)&31u,b=raw&31u;
    palette[i]=(uint16_t)((raw&0x8000u)|((r>>1)<<10)|((g>>1)<<5)|(b>>1));
   }else palette[i]=raw;
  }
  last_dimmed=dimmed;
 }
 /* Selection stripe follows the row selected in the Saroo-like listing. */
 /* Clear old stripe when changing screens, not just while in a menu. */
 for(i=0;i<9u;i++){
  unsigned y=48u+i*8u;
  rect(2u,y,317u,y+8u,6u);
 }
 if(index<9u &&
    (ui->page==UI_MAIN || ui->page==UI_ROMS ||
     ui->page==UI_OPTIONS || ui->page==UI_PAUSE)){
  /* Main menu has a 2-line heading; list begins at line 8 (64 px). */
  unsigned y=(ui->page==UI_MAIN?64u:48u)+index*8u;
  rect(3u,y,317u,y+8u,21u);
  rect(3u,y,6u,y+8u,20u);
 }
}
