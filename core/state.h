#ifndef MEGAORBIT_STATE_H
#define MEGAORBIT_STATE_H
#include "megacore.h"
#include <stdint.h>
#define MEGAORBIT_STATE_VERSION 1u
/* CPU+RAM+IO snapshot; does not capture complete VDP, Z80 or sound. */
typedef struct {
 uint32_t magic, version, rom_size, rom_fingerprint;
 m68k_t cpu;
 uint8_t ram[MD_RAM_SIZE], io[0x20], tmss[4];
 uint32_t fingerprint;
} md_state_t;
int md_state_save(md_state_t *state,const m68k_t *cpu,const md_bus_t *bus);
int md_state_load(const md_state_t *state,m68k_t *cpu,md_bus_t *bus);
#endif
