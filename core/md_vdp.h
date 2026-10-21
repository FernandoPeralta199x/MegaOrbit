#ifndef MEGAORBIT_VDP_H
#define MEGAORBIT_VDP_H
#include <stdint.h>
typedef struct {
 uint8_t vram[65536],regs[32];uint16_t cram[64],vsram[40],address,first;
 uint8_t code,pending;unsigned writes;
} md_vdp_t;
void md_vdp_init(md_vdp_t *v);
void md_vdp_ctrl(md_vdp_t *v,uint16_t data);
void md_vdp_data(md_vdp_t *v,uint16_t data);
void md_vdp_demo(md_vdp_t *v);
void md_vdp_render(const md_vdp_t *v,uint16_t *out,unsigned stride,unsigned w,unsigned h);
#endif
