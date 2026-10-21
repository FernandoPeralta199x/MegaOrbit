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
    bus->io[1] = 0xA0u; /* NTSC overseas, no expansion (version byte) */
    bus->io[3] = 0x7Fu; /* Controller port A, unpressed */
    bus->io[5] = 0x7Fu; /* Controller port B, unpressed */
    bus->io[7] = 0x7Fu; /* Controller port C, unpressed */
}

mc_result_t
md_read8(const md_bus_t *bus, uint32_t address, uint8_t *value)
{
    address &= BUS_MASK;
    if (address >= 0x00A10000u && address <= 0x00A1001Fu) {
        *value = bus->io[address - 0x00A10000u];
        return MC_OK;
    }
    if (address >= 0x00A14000u && address <= 0x00A14003u) {
        *value = bus->tmss[address - 0x00A14000u];
        return MC_OK;
    }
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
    if (address >= 0x00A10000u && address <= 0x00A1001Fu) {
        bus->io[address - 0x00A10000u] = value;
        return MC_OK;
    }
    if (address >= 0x00A14000u && address <= 0x00A14003u) {
        bus->tmss[address - 0x00A14000u] = value;
        return MC_OK;
    }
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
    if ((address & BUS_MASK) < MD_RAM_START &&
        !(((address & BUS_MASK) >= 0x00A10000u && (address & BUS_MASK) <= 0x00A1001Eu) ||
          ((address & BUS_MASK) >= 0x00A14000u && (address & BUS_MASK) <= 0x00A14002u)))
        return MC_ERR_UNMAPPED;
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
    /* TST.W/TST.L (xxx).L: common boot-time system register probes. */
    if (op == 0x4A79u || op == 0x4AB9u) {
        uint32_t addr, data32 = 0;
        uint16_t data16 = 0;
        r = fetch32(cpu, bus, &addr);
        if (r != MC_OK) return fail(cpu, r);
        if (op == 0x4AB9u) {
            r = md_read32(bus, addr, &data32);
            if (r != MC_OK) return fail(cpu, r);
            cpu->sr = (uint16_t)((cpu->sr & 0xFFF0u) |
                (data32 == 0 ? SR_ZERO : 0u) |
                ((data32 & 0x80000000u) ? SR_NEGATIVE : 0u));
        } else {
            r = md_read16(bus, addr, &data16);
            if (r != MC_OK) return fail(cpu, r);
            cpu->sr = (uint16_t)((cpu->sr & 0xFFF0u) |
                (data16 == 0 ? SR_ZERO : 0u) |
                ((data16 & 0x8000u) ? SR_NEGATIVE : 0u));
        }
        cpu->cycles += op == 0x4AB9u ? 20u : 16u;
        return MC_OK;
    }
    /* Bcc: original 68000 8-bit and 16-bit displacements (no long Bcc).
       This handles conditions including BNE/BEQ used in Sonic's boot. */
    if ((op & 0xF000u) == 0x6000u && (op & 0x0F00u) >= 0x0200u) {
        uint16_t ccr = cpu->sr & 0x1Fu;
        unsigned condition = (op >> 8u) & 15u;
        unsigned n = (ccr & SR_NEGATIVE) != 0u, z = (ccr & SR_ZERO) != 0u;
        unsigned v = (ccr & SR_OVERFLOW) != 0u, carry = (ccr & SR_CARRY) != 0u;
        int taken = 0;
        uint8_t d8 = (uint8_t)op;
        if (d8 == 0u) {
            uint16_t ext;
            r = fetch16(cpu, bus, &ext);
            if (r != MC_OK) return fail(cpu, r);
            displacement = (int16_t)ext;
        } else displacement = (int8_t)d8;
        switch(condition) {
        case 2: taken = !carry && !z; break;             /* BHI */
        case 3: taken = carry || z; break;               /* BLS */
        case 4: taken = !carry; break;                   /* BCC */
        case 5: taken = carry; break;                    /* BCS */
        case 6: taken = !z; break;                       /* BNE */
        case 7: taken = z; break;                        /* BEQ */
        case 8: taken = !v; break;                       /* BVC */
        case 9: taken = v; break;                        /* BVS */
        case 10: taken = !n; break;                      /* BPL */
        case 11: taken = n; break;                       /* BMI */
        case 12: taken = (n == v); break;                /* BGE */
        case 13: taken = (n != v); break;                /* BLT */
        case 14: taken = !z && (n == v); break;          /* BGT */
        case 15: taken = z || (n != v); break;           /* BLE */
        default: break;
        }
        if (taken) cpu->pc = (cpu->pc + (uint32_t)displacement) & BUS_MASK;
        cpu->cycles += taken ? 10u : (d8 == 0u ? 12u : 8u);
        return MC_OK;
    }
    /* LEA (d16,PC),An: 68000 uses the extension word address as its PC base. */
    if ((op & 0xF1FFu) == 0x41FAu) {
        uint16_t ext;
        uint32_t ext_pc = cpu->pc;
        r = fetch16(cpu, bus, &ext);
        if (r != MC_OK) return fail(cpu, r);
        cpu->a[(op >> 9u) & 7u] =
            (ext_pc + (uint32_t)(int32_t)(int16_t)ext) & BUS_MASK;
        cpu->cycles += 8u;
        return MC_OK;
    }
    /* MOVEM.{W,L} (An)+,register-list; transfer registers in D0..A7 order.
       Includes address update and word sign-extension. */
    if ((op & 0xFFB8u) == 0x4C98u) {
        uint16_t mask;
        unsigned reg, addr_reg = op & 7u, long_size = (op & 0x40u) != 0u;
        uint32_t addr;
        r = fetch16(cpu, bus, &mask);
        if (r != MC_OK) return fail(cpu, r);
        addr = cpu->a[addr_reg];
        for (reg = 0; reg < 16u; ++reg) {
            if (mask & (1u << reg)) {
                uint32_t val;
                if (long_size) r = md_read32(bus, addr, &val);
                else {
                    uint16_t word;
                    r = md_read16(bus, addr, &word);
                    val = (uint32_t)(int32_t)(int16_t)word;
                }
                if (r != MC_OK) return fail(cpu, r);
                if (reg < 8u) cpu->d[reg] = val;
                else cpu->a[reg - 8u] = val;
                addr += long_size ? 4u : 2u;
                cpu->cycles += long_size ? 8u : 4u;
            }
        }
        cpu->a[addr_reg] = addr & BUS_MASK;
        cpu->cycles += 12u;
        return MC_OK;
    }
    /* MOVE.B (d16,An),Dn: preserve other data-register bits. */
    if ((op & 0xF1F8u) == 0x1028u) {
        uint16_t ext;
        uint8_t data;
        unsigned src = op & 7u, dest = (op >> 9u) & 7u;
        r = fetch16(cpu, bus, &ext);
        if (r != MC_OK) return fail(cpu, r);
        r = md_read8(bus, (cpu->a[src] + (uint32_t)(int32_t)(int16_t)ext) & BUS_MASK, &data);
        if (r != MC_OK) return fail(cpu, r);
        cpu->d[dest] = (cpu->d[dest] & 0xFFFFFF00u) | data;
        cpu->sr = (uint16_t)((cpu->sr & 0xFFF0u) | (data == 0 ? SR_ZERO : 0u) |
                             ((data & 0x80u) ? SR_NEGATIVE : 0u));
        cpu->cycles += 12u;
        return MC_OK;
    }
    /* ANDI.B #imm,Dn */
    if ((op & 0xFFF8u) == 0x0200u) {
        uint16_t ext;
        unsigned dest = op & 7u;
        uint8_t data;
        r = fetch16(cpu, bus, &ext);
        if (r != MC_OK) return fail(cpu, r);
        data = (uint8_t)(cpu->d[dest] & (uint8_t)ext);
        cpu->d[dest] = (cpu->d[dest] & 0xFFFFFF00u) | data;
        cpu->sr = (uint16_t)((cpu->sr & 0xFFF0u) | (data == 0 ? SR_ZERO : 0u) |
                             ((data & 0x80u) ? SR_NEGATIVE : 0u));
        cpu->cycles += 8u;
        return MC_OK;
    }
    /* MOVE.L #imm,(d16,An), including TMSS 'SEGA' unlock register. */
    if ((op & 0xF1FFu) == 0x217Cu) {
        uint32_t data;
        uint16_t ext;
        unsigned dest = (op >> 9u) & 7u;
        r = fetch32(cpu, bus, &data);
        if (r != MC_OK) return fail(cpu, r);
        r = fetch16(cpu, bus, &ext);
        if (r != MC_OK) return fail(cpu, r);
        r = md_write32(bus, (cpu->a[dest] + (uint32_t)(int32_t)(int16_t)ext) & BUS_MASK, data);
        if (r != MC_OK) return fail(cpu, r);
        cpu->sr = (uint16_t)((cpu->sr & 0xFFF0u) | (data == 0 ? SR_ZERO : 0u) |
                             ((data & 0x80000000u) ? SR_NEGATIVE : 0u));
        cpu->cycles += 28u;
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
