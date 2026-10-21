#include "megacore.h"
#include <string.h>

#define BUS_MASK 0x00FFFFFFu
#define MD_RAM_START 0x00FF0000u
#define SR_NEGATIVE 0x0008u
#define SR_ZERO 0x0004u
#define SR_OVERFLOW 0x0002u
#define SR_CARRY 0x0001u

void
md_bus_init(md_bus_t *bus, const uint8_t *rom, size_t rom_size)
{
    memset(bus, 0, sizeof(*bus));
    bus->rom = rom;
    bus->rom_size = rom_size;
}

mc_result_t
md_read8(const md_bus_t *bus, uint32_t address, uint8_t *value)
{
    address &= BUS_MASK;
    if (address >= MD_RAM_START) {
        *value = bus->ram[address & (MD_RAM_SIZE - 1u)];
        return MC_OK;
    }
    if (address < MD_ROM_WINDOW_SIZE && bus->rom != NULL && (size_t)address < bus->rom_size) {
        *value = bus->rom[address];
        return MC_OK;
    }
    return MC_ERR_UNMAPPED;
}

mc_result_t
md_read16(const md_bus_t *bus, uint32_t address, uint16_t *value)
{
    uint8_t hi, lo;
    mc_result_t r;
    if (address & 1u) return MC_ERR_ADDRESS;
    r = md_read8(bus, address, &hi);
    if (r != MC_OK) return r;
    r = md_read8(bus, address + 1u, &lo);
    if (r != MC_OK) return r;
    *value = (uint16_t)(((uint16_t)hi << 8u) | lo);
    return MC_OK;
}

mc_result_t
md_read32(const md_bus_t *bus, uint32_t address, uint32_t *value)
{
    uint16_t hi, lo;
    mc_result_t r = md_read16(bus, address, &hi);
    if (r != MC_OK) return r;
    r = md_read16(bus, address + 2u, &lo);
    if (r != MC_OK) return r;
    *value = ((uint32_t)hi << 16u) | lo;
    return MC_OK;
}

mc_result_t
md_write8(md_bus_t *bus, uint32_t address, uint8_t value)
{
    address &= BUS_MASK;
    if (address < MD_RAM_START) return MC_ERR_UNMAPPED;
    bus->ram[address & (MD_RAM_SIZE - 1u)] = value;
    return MC_OK;
}

mc_result_t
md_write16(md_bus_t *bus, uint32_t address, uint16_t value)
{
    mc_result_t r;
    if (address & 1u) return MC_ERR_ADDRESS;
    /* All currently writable memory is RAM, so a paired write is safe. */
    if ((address & BUS_MASK) < MD_RAM_START) return MC_ERR_UNMAPPED;
    r = md_write8(bus, address, (uint8_t)(value >> 8u));
    if (r != MC_OK) return r;
    return md_write8(bus, address + 1u, (uint8_t)value);
}

mc_result_t
md_write32(md_bus_t *bus, uint32_t address, uint32_t value)
{
    mc_result_t r = md_write16(bus, address, (uint16_t)(value >> 16u));
    if (r != MC_OK) return r;
    return md_write16(bus, address + 2u, (uint16_t)value);
}

static mc_result_t
fail(m68k_t *cpu, mc_result_t error)
{
    cpu->error = error;
    cpu->halted = 1;
    return error;
}

static mc_result_t
fetch16(m68k_t *cpu, md_bus_t *bus, uint16_t *word)
{
    mc_result_t r = md_read16(bus, cpu->pc, word);
    if (r == MC_OK) cpu->pc = (cpu->pc + 2u) & BUS_MASK;
    return r;
}

static mc_result_t
fetch32(m68k_t *cpu, md_bus_t *bus, uint32_t *dword)
{
    mc_result_t r = md_read32(bus, cpu->pc, dword);
    if (r == MC_OK) cpu->pc = (cpu->pc + 4u) & BUS_MASK;
    return r;
}

static mc_result_t
push32(m68k_t *cpu, md_bus_t *bus, uint32_t value)
{
    uint32_t new_sp = (cpu->a[7] - 4u) & BUS_MASK;
    mc_result_t r = md_write32(bus, new_sp, value);
    if (r == MC_OK) cpu->a[7] = new_sp;
    return r;
}

static mc_result_t
pop32(m68k_t *cpu, md_bus_t *bus, uint32_t *value)
{
    mc_result_t r = md_read32(bus, cpu->a[7], value);
    if (r == MC_OK) cpu->a[7] = (cpu->a[7] + 4u) & BUS_MASK;
    return r;
}

mc_result_t
m68k_reset(m68k_t *cpu, const md_bus_t *bus)
{
    uint32_t sp, pc;
    mc_result_t r;
    memset(cpu, 0, sizeof(*cpu));
    cpu->sr = 0x2700u;
    r = md_read32(bus, 0u, &sp);
    if (r != MC_OK) return fail(cpu, r);
    r = md_read32(bus, 4u, &pc);
    if (r != MC_OK) return fail(cpu, r);
    cpu->a[7] = sp & BUS_MASK;
    cpu->pc = pc & BUS_MASK;
    return MC_OK;
}

mc_result_t
m68k_step(m68k_t *cpu, md_bus_t *bus)
{
    uint16_t op;
    uint32_t target;
    mc_result_t r;
    int32_t displacement;

    if (cpu->halted) return MC_ERR_HALTED;
    r = fetch16(cpu, bus, &op);
    if (r != MC_OK) return fail(cpu, r);
    cpu->last_opcode = op;

    if (op == 0x4E71u) { /* NOP */
        cpu->cycles += 4u;
        return MC_OK;
    }
    if ((op & 0xF100u) == 0x7000u) { /* MOVEQ #imm8,Dn */
        uint32_t n = (op >> 9u) & 7u;
        int32_t signed_value = (int32_t)(int8_t)(op & 0xFFu);
        cpu->d[n] = (uint32_t)signed_value;
        cpu->sr &= (uint16_t)~(SR_NEGATIVE | SR_ZERO | SR_OVERFLOW | SR_CARRY);
        if (signed_value == 0) cpu->sr |= SR_ZERO;
        if (signed_value < 0) cpu->sr |= SR_NEGATIVE;
        cpu->cycles += 4u;
        return MC_OK;
    }
    if ((op & 0xFF00u) == 0x6000u || (op & 0xFF00u) == 0x6100u) {
        /* BRA and BSR only, 8-bit/16-bit displacements, not 68020 long branches. */
        uint8_t d8 = (uint8_t)(op & 0xFFu);
        if (d8 == 0u) {
            uint16_t ext;
            r = fetch16(cpu, bus, &ext);
            if (r != MC_OK) return fail(cpu, r);
            displacement = (int32_t)(int16_t)ext;
        } else {
            displacement = (int32_t)(int8_t)d8;
        }
        target = (cpu->pc + (uint32_t)displacement) & BUS_MASK;
        if ((op & 0xFF00u) == 0x6100u) {
            r = push32(cpu, bus, cpu->pc);
            if (r != MC_OK) return fail(cpu, r);
            cpu->cycles += 18u;
        } else {
            cpu->cycles += 10u;
        }
        cpu->pc = target;
        return MC_OK;
    }
    if (op == 0x4E75u) { /* RTS */
        r = pop32(cpu, bus, &target);
        if (r != MC_OK) return fail(cpu, r);
        cpu->pc = target & BUS_MASK;
        cpu->cycles += 16u;
        return MC_OK;
    }
    if (op == 0x4EF9u || op == 0x4EB9u) { /* JMP.L abs / JSR.L abs */
        r = fetch32(cpu, bus, &target);
        if (r != MC_OK) return fail(cpu, r);
        if (op == 0x4EB9u) {
            r = push32(cpu, bus, cpu->pc);
            if (r != MC_OK) return fail(cpu, r);
            cpu->cycles += 20u;
        } else {
            cpu->cycles += 10u;
        }
        cpu->pc = target & BUS_MASK;
        return MC_OK;
    }
    /* Deliberate fail-fast instead of pretending to emulate illegal exceptions. */
    return fail(cpu, MC_ERR_OPCODE);
}

const char *
mc_result_name(mc_result_t result)
{
    switch (result) {
    case MC_OK: return "OK";
    case MC_ERR_ADDRESS: return "ADDRESS_ERROR";
    case MC_ERR_UNMAPPED: return "UNMAPPED_BUS";
    case MC_ERR_OPCODE: return "UNIMPLEMENTED_OPCODE";
    case MC_ERR_HALTED: return "HALTED";
    default: return "UNKNOWN";
    }
}
