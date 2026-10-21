#ifndef MD_EMULATION_H
#define MD_EMULATION_H
#include <stdint.h>
#include <stddef.h>
#define MD_AUDIO_RATE 22050u
typedef struct {
 uint8_t ram[8192],a,b,f,halted,last_port,last_data;
 uint16_t pc,sp;
 uint32_t cycles;unsigned out_count;
} md_z80_t;
void md_z80_init(md_z80_t *z);
int md_z80_step(md_z80_t *z);
typedef struct {
 uint16_t period[3],noise_lfsr;
 uint8_t volume[4],noise_control,latched;
 uint32_t phase[4];
} md_psg_t;
void md_psg_init(md_psg_t *p);
void md_psg_write(md_psg_t *p,uint8_t byte);
int32_t md_psg_sample(md_psg_t *p);
typedef struct {
 uint8_t reg[2][256],key[6];
 uint32_t phase[6],modphase[6];
} md_ym_lite_t;
void md_ym_init(md_ym_lite_t *f);
void md_ym_write(md_ym_lite_t *f,unsigned part,uint8_t addr,uint8_t value);
int32_t md_ym_sample(md_ym_lite_t *f);
void md_audio_generate_demo(int16_t *out,size_t count);
#endif
