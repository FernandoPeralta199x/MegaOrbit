#include "md_vdp.h"
#include <string.h>
void md_vdp_init(md_vdp_t *v){memset(v,0,sizeof(*v));v->regs[15]=2;}
void md_vdp_ctrl(md_vdp_t *v,uint16_t x){
 if(!v->pending){
  if((x&0xc000u)==0x8000u){unsigned r=(x>>8)&31u;v->regs[r]=(uint8_t)x;return;}
  v->first=x;v->pending=1;
 }else{
  v->address=(uint16_t)((v->first&0x3fffu)|((x&3u)<<14));
  v->code=(uint8_t)(((v->first>>14)&3u)|((x>>2)&0x3cu));
  v->pending=0;
 }
}
void md_vdp_data(md_vdp_t *v,uint16_t x){
 unsigned ix=v->address>>1;
 if((v->code&15u)==1u){
  v->vram[v->address]=(uint8_t)(x>>8);
  v->vram[(uint16_t)(v->address+1u)]=(uint8_t)x;v->writes++;
 }else if((v->code&15u)==3u){
  v->cram[ix&63u]=x&0x0eeeu;v->writes++;
 }else if((v->code&15u)==5u){
  v->vsram[ix%40u]=x;v->writes++;
 }
 v->address=(uint16_t)(v->address+v->regs[15]);
}
static uint16_t rgb(uint16_t c){
 unsigned r=(c>>1)&7u,g=(c>>5)&7u,b=(c>>9)&7u;
 return (uint16_t)(0x8000u|((b*31u/7u)<<10)|((g*31u/7u)<<5)|(r*31u/7u));
}
void md_vdp_demo(md_vdp_t *v){
 unsigned tx,ty,id,y,x,off;
 md_vdp_init(v);v->regs[2]=0x30u;
 for(id=0;id<64u;id++)
  v->cram[id]=(uint16_t)((((id>>1)&7u)<<9)|(((id>>3)&7u)<<5)|((id&7u)<<1));
 for(id=0;id<16u;id++)
  for(y=0;y<8u;y++)for(x=0;x<8u;x+=2u){
   uint8_t a=(uint8_t)(((x+y+id)&7u)+1u),b=(uint8_t)(((x+y+id+3u)&7u)+1u);
   off=id*32u+y*4u+x/2u;v->vram[off]=(uint8_t)((a<<4)|b);
  }
 /* Pattern name table: H40 64 cells per row. */
 for(ty=0;ty<28u;ty++)for(tx=0;tx<40u;tx++){
  uint16_t entry=(uint16_t)(((tx+ty)&15u)|(((tx/10u)&3u)<<13));
  off=0xc000u+(ty*64u+tx)*2u;
  v->vram[off]=(uint8_t)(entry>>8);v->vram[off+1u]=(uint8_t)entry;
 }
}
void md_vdp_render(const md_vdp_t *v,uint16_t *out,unsigned stride,unsigned w,unsigned h){
 unsigned x,y,base=((unsigned)(v->regs[2]&0x38u))<<10;
 for(y=0;y<h;y++)for(x=0;x<w;x++){
  unsigned tx=(x>>3)&63u,ty=(y>>3)&31u;
  unsigned addr=(base+(ty*64u+tx)*2u)&65535u;
  uint16_t e=(uint16_t)(((uint16_t)v->vram[addr]<<8)|v->vram[(addr+1u)&65535u]);
  unsigned tile=e&0x7ffu;
  unsigned fx=(e&0x0800u)?(7u-(x&7u)):(x&7u);
  unsigned fy=(e&0x1000u)?(7u-(y&7u)):(y&7u);
  unsigned off=(tile*32u+fy*4u+(fx>>1))&65535u;
  uint8_t index=(fx&1u)?(v->vram[off]&15u):(v->vram[off]>>4);
  unsigned palette=((e>>9)&48u)|index;
  out[y*stride+x]=rgb(v->cram[palette&63u]);
 }
}
