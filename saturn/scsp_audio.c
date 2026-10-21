#include <yaul.h>
#include <stdint.h>
#include "md_emulation.h"
#include "scsp_audio.h"
/* Saturn SCSP direct registers: 32 slots * 0x20 bytes, control @ +0x400.
 * Source: Sega SCSP User's Manual, register map section 4.1.
 * PCM in sound RAM 0x10000, 4096 mono 16-bit samples at ~22050Hz.
 * This is a short LOOPING audio test, not streamed emulation output.
 */
#define PCM_LENGTH 4096u
static int16_t pcm[PCM_LENGTH];
void megaorbit_scsp_stop(void){
 volatile uint16_t *slot=(volatile uint16_t *)0x25b00000u;
 slot[0]=0u; /* KEY OFF */
 slot[0]=0x1000u; /* execute KEY OFF */
}
void megaorbit_scsp_play_test(void){
 unsigned i;
 volatile uint16_t *const slot=(volatile uint16_t *)0x25b00000u;
 volatile uint16_t *const master=(volatile uint16_t *)0x25b00400u;
 volatile uint16_t *const ram=(volatile uint16_t *)0x25a10000u;
 md_audio_generate_demo(pcm,PCM_LENGTH);
 /* This demo takes exclusive use of slot 0. We do not install
  * a 68EC000 sound driver, and avoid overwriting boot ROM/code. */
 megaorbit_scsp_stop();
 for(i=0;i<PCM_LENGTH;i++)ram[i]=(uint16_t)pcm[i];
 *master=0x020fu; /* MEM4MB=1 and MVOL=15 */
 slot[0]=0x0021u; /* normal looping and SA high nibble=1 */
 slot[1]=0x0000u; /* SA low: 0x10000 */
 slot[2]=0u; /* LSA */
 slot[3]=(uint16_t)(PCM_LENGTH-1u); /* LEA */
 slot[4]=0x001fu; /* instant attack */
 slot[5]=0x001fu; /* fast release */
 slot[6]=0x0000u; /* TL=0 */
 slot[7]=0u; /* FM modulation off */
 slot[8]=0x7800u; /* OCT=-1, FNS=0; 22050Hz */
 slot[9]=0u;
 slot[10]=0u;
 slot[11]=0xe000u; /* direct level=7 center pan */
 slot[0]=0x1821u; /* KYONEX KYONB LOOP and SA high */
}
