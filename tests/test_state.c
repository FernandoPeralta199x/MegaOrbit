#include "../core/state.h"
#include "../core/homebrew.h"
#include <stdio.h>
#include <string.h>
static md_bus_t bus;
static m68k_t cpu;
static md_state_t state;
static uint8_t rom[ORBIT_CART_SIZE],other[ORBIT_CART_SIZE];
#define CK(c) do {if(!(c)){printf("FAIL at %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
 orbit_cart_build(rom);
 md_bus_init(&bus,rom,sizeof(rom));
 CK(m68k_reset(&cpu,&bus)==MC_OK);
 CK(md_write32(&bus,0xFF0010u,0xDEADBEEFu)==MC_OK);
 CK(md_state_save(&state,&cpu,&bus));
 cpu.d[0]=0x12345678u;
 CK(md_write32(&bus,0xFF0010u,0xAABBCCDDu)==MC_OK);
 CK(md_state_load(&state,&cpu,&bus));
 CK(cpu.d[0]==0u);
 {uint32_t x;CK(md_read32(&bus,0xFF0010u,&x)==MC_OK&&x==0xDEADBEEFu);}
 state.ram[0x10u]^=1u;
 CK(!md_state_load(&state,&cpu,&bus));
 state.ram[0x10u]^=1u;
 memcpy(other,rom,sizeof(rom));
 other[0x110u]^=1u;
 md_bus_init(&bus,other,sizeof(other));
 CK(!md_state_load(&state,&cpu,&bus));
 md_bus_init(&bus,rom,sizeof(rom));
 CK(md_state_load(&state,&cpu,&bus));
 puts("PASS: CPU/RAM snapshot, tampering detection, ROM fingerprint");
 return 0;
}
