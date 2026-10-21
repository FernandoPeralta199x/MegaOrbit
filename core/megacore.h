#ifndef MEGACORE_H
#define MEGACORE_H

#include <stddef.h>
#include <stdint.h>

#define MD_RAM_SIZE 65536u
#define MD_ROM_WINDOW_SIZE 0x400000u

/* This is a minimal development bus, NOT a complete Mega Drive memory map. */
typedef enum {
    MC_OK = 0,
    MC_ERR_ADDRESS,
    MC_ERR_UNMAPPED,
    MC_ERR_OPCODE,
    MC_ERR_HALTED
} mc_result_t;

typedef struct {
    const uint8_t *rom;
    size_t rom_size;
    uint8_t ram[MD_RAM_SIZE];
} md_bus_t;

typedef struct {
    uint32_t d[8];
    uint32_t a[8];
    uint32_t pc;
    uint32_t cycles;
    uint16_t sr;
    uint16_t last_opcode;
    mc_result_t error;
    uint8_t halted;
} m68k_t;

void md_bus_init(md_bus_t *bus, const uint8_t *rom, size_t rom_size);
mc_result_t md_read8(const md_bus_t *bus, uint32_t address, uint8_t *value);
mc_result_t md_read16(const md_bus_t *bus, uint32_t address, uint16_t *value);
mc_result_t md_read32(const md_bus_t *bus, uint32_t address, uint32_t *value);
mc_result_t md_write8(md_bus_t *bus, uint32_t address, uint8_t value);
mc_result_t md_write16(md_bus_t *bus, uint32_t address, uint16_t value);
mc_result_t md_write32(md_bus_t *bus, uint32_t address, uint32_t value);

/* Reads reset vectors at ROM offsets 0 and 4. */
mc_result_t m68k_reset(m68k_t *cpu, const md_bus_t *bus);
mc_result_t m68k_step(m68k_t *cpu, md_bus_t *bus);
const char *mc_result_name(mc_result_t result);

#endif
