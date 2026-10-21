/* A completely original Mega Drive 68000 test program.
 * It reads the virtual Mega Drive joypad IO port at A10003.
 * LEFT moves D1 -1, RIGHT moves D1 +1, then jumps back.
 * Graphics/targets are rendered by the Saturn test harness, not Genesis VDP.
 * It is deliberately not presented as a fully emulated Genesis game.
 */
#include "homebrew.h"
#include <string.h>
static void word(uint8_t *c,unsigned at,uint16_t v){
 c[at]=(uint8_t)(v>>8);c[at+1u]=(uint8_t)v;
}
static void longword(uint8_t *c,unsigned at,uint32_t v){
 word(c,at,(uint16_t)(v>>16));word(c,at+2u,(uint16_t)v);
}
void orbit_cart_build(uint8_t c[ORBIT_CART_SIZE]){
 memset(c,0,ORBIT_CART_SIZE);
 longword(c,0u,0x00FF0100u); /* A7 */
 longword(c,4u,0x00000200u); /* PC */
 memcpy(c+0x100u,"SEGA MEGA DRIVE ",16u);
 memcpy(c+0x120u,"MEGAORBIT HOMEBREW PADDLE",26u);
 memcpy(c+0x1F0u,"JUE",3u);
 word(c,0x200u,0x7220u); /* MOVEQ #32,D1 */
 word(c,0x202u,0x1039u);longword(c,0x204u,0x00A10003u); /* MOVE.B PAD,D0 */
 word(c,0x208u,0x0200u);word(c,0x20Au,0x0004u); /* ANDI.B #4,D0 */
 word(c,0x20Cu,0x6602u); /* BNE.s +2 */
 word(c,0x20Eu,0x5341u); /* SUBQ.W #1,D1 */
 word(c,0x210u,0x1039u);longword(c,0x212u,0x00A10003u);
 word(c,0x216u,0x0200u);word(c,0x218u,0x0008u); /* ANDI.B #8,D0 */
 word(c,0x21Au,0x6602u);
 word(c,0x21Cu,0x5241u); /* ADDQ.W #1,D1 */
 word(c,0x21Eu,0x60E2u); /* BRA.s -30 -> 0x202 */
}
