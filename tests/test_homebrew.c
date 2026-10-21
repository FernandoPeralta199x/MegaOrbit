#include "../core/homebrew.h"
#include "../core/megacore.h"
#include <stdio.h>
#include <string.h>
static md_bus_t bus;
static m68k_t cpu;
static uint8_t cart[ORBIT_CART_SIZE];
#define CK(c) do {if(!(c)){printf("FAIL %s line %d\n",#c,__LINE__);return 1;}}while(0)
static int tick(void){
 unsigned i;
 for(i=0;i<9u;i++)if(m68k_step(&cpu,&bus)!=MC_OK)return 0;
 return 1;
}
int main(void){
 unsigned i;
 orbit_cart_build(cart);
 CK(!memcmp(cart+0x100u,"SEGA MEGA DRIVE",15u));
 md_bus_init(&bus,cart,sizeof(cart));
 CK(m68k_reset(&cpu,&bus)==MC_OK && cpu.pc==0x200u);
 CK(m68k_step(&cpu,&bus)==MC_OK && cpu.d[1]==32u);
 bus.io[3]=0x7Fu; /* no direction */
 for(i=0;i<3u;i++)CK(tick());
 CK(cpu.d[1]==32u);
 bus.io[3]=0x7Bu; /* LEFT active low */
 for(i=0;i<6u;i++)CK(tick());
 CK(cpu.d[1]<32u);
 unsigned left=(unsigned)(cpu.d[1]&0xFFFFu);
 bus.io[3]=0x77u; /* RIGHT active low */
 for(i=0;i<6u;i++)CK(tick());
 CK((cpu.d[1]&0xFFFFu)>left);
 printf("PASS: homebrew 68000 controller loop input/step; D1=%lu\n",(unsigned long)cpu.d[1]);
 return 0;
}
