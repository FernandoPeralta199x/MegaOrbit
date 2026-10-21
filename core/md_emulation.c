#include "md_emulation.h"
#include <string.h>
void md_z80_init(md_z80_t *z){memset(z,0,sizeof(*z));z->sp=0x1fffu;}
static uint8_t fetch(md_z80_t *z){uint8_t b=z->ram[z->pc&8191u];z->pc++;return b;}
int md_z80_step(md_z80_t *z){
 uint8_t op,lo,hi;
 if(z->halted)return 0;
 op=fetch(z);
 switch(op){
 case 0x00:z->cycles+=4;return 1;
 case 0x3e:z->a=fetch(z);z->cycles+=7;return 1;
 case 0x06:z->b=fetch(z);z->cycles+=7;return 1;
 case 0xaf:z->a=0;z->f=0x44;z->cycles+=4;return 1;
 case 0x3c:z->a++;z->f=(uint8_t)(z->a==0?0x40:0);z->cycles+=4;return 1;
 case 0x32:lo=fetch(z);hi=fetch(z);z->ram[(((unsigned)hi<<8)|lo)&8191u]=z->a;z->cycles+=13;return 1;
 case 0x3a:lo=fetch(z);hi=fetch(z);z->a=z->ram[(((unsigned)hi<<8)|lo)&8191u];z->cycles+=13;return 1;
 case 0xd3:z->last_port=fetch(z);z->last_data=z->a;z->out_count++;z->cycles+=11;return 1;
 case 0xc3:lo=fetch(z);hi=fetch(z);z->pc=(uint16_t)(((unsigned)hi<<8)|lo);z->cycles+=10;return 1;
 case 0x10:{int8_t d=(int8_t)fetch(z);z->b--;
  if(z->b){z->pc=(uint16_t)(z->pc+d);z->cycles+=13;}
  else { z->cycles+=8; }
  return 1;}
 case 0x76:z->halted=1;z->cycles+=4;return 1;
 default:z->halted=1;return -1;
 }
}
static const int16_t amp[16]={8192,6507,5168,4105,3260,2590,2057,1634,1298,1031,819,651,517,411,326,0};
void md_psg_init(md_psg_t *p){
 unsigned i;memset(p,0,sizeof(*p));for(i=0;i<4;i++)p->volume[i]=15u;
 for(i=0;i<3;i++)p->period[i]=1u;
 p->noise_lfsr=0x8000;
}
void md_psg_write(md_psg_t *p,uint8_t data){
 unsigned ch;
 if(data&0x80u){
  p->latched=(data>>4)&7u;ch=p->latched>>1;
  if(p->latched&1u)p->volume[ch]=data&15u;
  else if(ch==3u){p->noise_control=data&7u;p->noise_lfsr=0x8000u;}
  else p->period[ch]=(uint16_t)((p->period[ch]&0x3f0u)|(data&15u));
 }else{
  ch=p->latched>>1;
  if(p->latched&1u)p->volume[ch]=data&15u;
  else if(ch<3u)p->period[ch]=(uint16_t)((p->period[ch]&15u)|((data&63u)<<4));
 }
}
int32_t md_psg_sample(md_psg_t *p){
 unsigned ch;int32_t mix=0;
 for(ch=0;ch<3;ch++){
  uint32_t period=p->period[ch]?p->period[ch]:1u;
  uint32_t step=(uint32_t)(((uint64_t)3579545u*65536u)/(32u*period*MD_AUDIO_RATE));
  if(step>65535u)step=65535u;
  p->phase[ch]=(p->phase[ch]+step)&65535u;
  mix+=(p->phase[ch]<32768u?1:-1)*amp[p->volume[ch]];
 }
 {unsigned mode=p->noise_control&3u;
  unsigned period=mode==3u?(p->period[2]?p->period[2]:1u):(16u<<mode);
  uint32_t step=(uint32_t)(((uint64_t)3579545u*65536u)/(32u*period*MD_AUDIO_RATE));
  if(step>65535u)step=65535u;
  p->phase[3]+=step;
  if(p->phase[3]>=65536u){
   uint16_t tap=(p->noise_control&4u)?(uint16_t)((p->noise_lfsr^(p->noise_lfsr>>3))&1u):(p->noise_lfsr&1u);
   p->noise_lfsr=(uint16_t)((p->noise_lfsr>>1)|(tap<<15));
   p->phase[3]-=65536u;
  }
  mix+=(p->noise_lfsr&1u?1:-1)*amp[p->volume[3]];
 }
 return mix/4;
}
/* Educational 2-operator FM approximation, NOT a complete YM2612 emulator. */
static int32_t wave(uint32_t phase){
 unsigned x=(phase>>8)&255u;
 int32_t t=(x<128u)?(int32_t)x:(int32_t)(255u-x);
 int32_t n=t<64?t:127-t;
 int32_t curve=n*(192-n)/16;
 return x<128u?curve:-curve;
}
void md_ym_init(md_ym_lite_t *f){memset(f,0,sizeof(*f));}
void md_ym_write(md_ym_lite_t *f,unsigned part,uint8_t addr,uint8_t val){
 unsigned channel;
 if(part>1u)return;
 f->reg[part][addr]=val;
 if(part==0u && addr==0x28u){
  channel=(val&3u)+((val&4u)?3u:0u);
  if(channel<6u)f->key[channel]=(uint8_t)((val&0xf0u)!=0u);
 }
}
int32_t md_ym_sample(md_ym_lite_t *f){
 int32_t total=0;unsigned c;
 for(c=0;c<6u;c++){
  unsigned part=c/3u,slot=c%3u;
  unsigned fnum=(unsigned)f->reg[part][0xa0u+slot] |
        ((unsigned)(f->reg[part][0xa4u+slot]&7u)<<8);
  unsigned block=(f->reg[part][0xa4u+slot]>>3)&7u;
  uint32_t hz,step;int32_t modulation;
  if(!f->key[c]||!fnum)continue;
  hz=((uint32_t)fnum*53u*(1u<<block))>>10;
  if(hz>7000u)hz=7000u;
  step=(hz*65536u)/MD_AUDIO_RATE;
  f->modphase[c]+=step*2u;
  modulation=wave(f->modphase[c]);
  f->phase[c]+=step;
  total+=wave(f->phase[c]+(uint32_t)(modulation*4))*2;
 }
 return total/6;
}
void md_audio_generate_demo(int16_t *out,size_t count){
 md_psg_t p;md_ym_lite_t f;size_t i;
 md_psg_init(&p);md_ym_init(&f);
 md_psg_write(&p,0x84);md_psg_write(&p,0x1b);md_psg_write(&p,0x90);
 md_psg_write(&p,0xa0);md_psg_write(&p,0x15);md_psg_write(&p,0xb4);
 md_psg_write(&p,0xc5);md_psg_write(&p,0x10);md_psg_write(&p,0xd7);
 md_ym_write(&f,0,0xa0,0x20);md_ym_write(&f,0,0xa4,0x22);md_ym_write(&f,0,0x28,0xf0);
 for(i=0;i<count;i++){
  int32_t sample=md_psg_sample(&p)+md_ym_sample(&f);
  if(sample>32767)sample=32767;
  if(sample< -32768)sample=-32768;
  out[i]=(int16_t)sample;
 }
}
