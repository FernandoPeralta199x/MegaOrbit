#include "../core/md_emulation.h"
#include "../core/md_vdp.h"
#include <stdio.h>
#include <string.h>
static md_vdp_t v;
static md_z80_t z;
static md_psg_t p;
static md_ym_lite_t f;
static int16_t audio[1024];
static uint16_t screen[64u*48u];
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
 unsigned i,nonzero=0,colors=0;
 md_z80_init(&z);
 z.ram[0]=0x3e;z.ram[1]=0x5a; /* LD A,5a */
 z.ram[2]=0xd3;z.ram[3]=0x7f; /* OUT (7f),A */
 z.ram[4]=0x06;z.ram[5]=0x02; /* LD B,2 */
 z.ram[6]=0x10;z.ram[7]=0xfe; /* DJNZ -2 */
 z.ram[8]=0x76; /* HALT */
 for(i=0;i<8u&&!z.halted;i++) CHECK(md_z80_step(&z)==1);
 CHECK(z.halted && z.a==0x5au && z.last_port==0x7fu);
 CHECK(z.last_data==0x5au && z.out_count==1);
 md_z80_init(&z);z.ram[0]=0xff;CHECK(md_z80_step(&z)==-1);
 md_psg_init(&p);md_psg_write(&p,0x84);md_psg_write(&p,0x1b);
 md_psg_write(&p,0x90);for(i=0;i<300u;i++)if(md_psg_sample(&p))nonzero++;
 CHECK(nonzero>0);
 md_ym_init(&f);md_ym_write(&f,0,0xa0,0x20);
 md_ym_write(&f,0,0xa4,0x22);md_ym_write(&f,0,0x28,0xf0);
 CHECK(f.key[0]!=0);
 md_audio_generate_demo(audio,1024);
 nonzero=0;for(i=0;i<1024;i++)if(audio[i])nonzero++;
 CHECK(nonzero>200);
 md_vdp_init(&v);md_vdp_ctrl(&v,0x8f02);
 CHECK(v.regs[15]==2u);
 /* VRAM write address 0, CD 01 */
 md_vdp_ctrl(&v,0x4000);md_vdp_ctrl(&v,0x0000);
 md_vdp_data(&v,0x1234);
 CHECK(v.vram[0]==0x12 && v.vram[1]==0x34);
 CHECK(v.address==2 && v.writes==1);
 md_vdp_demo(&v);
 md_vdp_render(&v,screen,64,64,48);
 for(i=1;i<64u*48u;i++)if(screen[i]!=screen[0])colors++;
 CHECK(colors>100);
 printf("PASS: Z80 subset, PSG, FM approximation, SCSP sample generation and tiled VDP\n");
 return 0;
}
