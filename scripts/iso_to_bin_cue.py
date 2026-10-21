#!/usr/bin/env python3
"""Convert a Saturn 2048-byte-sector ISO into raw MODE1/2352 BIN+CUE.

The output includes CD EDC and Reed-Solomon ECC P/Q, not just zero padding.
No copyrighted firmware, BIOS, or commercial ROM is included.
"""
import argparse
from pathlib import Path

def bcd(value):
    return ((value // 10) << 4) | (value % 10)

def make_tables():
    fwd = [0] * 256
    back = [0] * 256
    edc = [0] * 256
    for i in range(256):
        j = i << 1
        if j & 0x100:
            j ^= 0x11D
        fwd[i] = j & 0xFF
        back[i ^ j] = i
        value = i
        for _ in range(8):
            value = (value >> 1) ^ (0xD8018001 if value & 1 else 0)
        edc[i] = value
    return fwd, back, edc

FWD, BACK, EDC = make_tables()

def ecc(source, majors, minors, mult, inc):
    size = majors * minors
    out = bytearray(majors * 2)
    for major in range(majors):
        index = (major >> 1) * mult + (major & 1)
        a = b = 0
        for _ in range(minors):
            value = source[index]
            index += inc
            if index >= size:
                index -= size
            a ^= value
            b ^= value
            a = FWD[a]
        a = BACK[FWD[a] ^ b]
        out[major] = a
        out[major + majors] = a ^ b
    return out

def raw_mode1_sector(lba, payload):
    if len(payload) != 2048:
        raise ValueError("ISO sector must be 2048 bytes")
    m, remain = divmod(lba + 150, 75 * 60)
    s, f = divmod(remain, 75)
    if m > 99:
        raise ValueError("Disc exceeds MSF addressing limit")
    sector = bytearray(2352)
    sector[0:12] = b"\x00" + (b"\xff" * 10) + b"\x00"
    sector[12:16] = bytes((bcd(m), bcd(s), bcd(f), 1))
    sector[16:2064] = payload
    checksum = 0
    for value in sector[:2064]:
        checksum = (checksum >> 8) ^ EDC[(checksum ^ value) & 0xFF]
    sector[2064:2068] = checksum.to_bytes(4, "little")
    # 2068:2076 remains zero (Mode1 reserved bytes)
    sector[2076:2248] = ecc(sector[12:2076], 86, 24, 2, 86)
    sector[2248:2352] = ecc(sector[12:2248], 52, 43, 86, 88)
    return sector

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--src", type=Path, required=True)
    p.add_argument("--dest", type=Path, required=True)
    p.add_argument("--name", default="MegaOrbit")
    a = p.parse_args()
    candidates = list(a.src.rglob("*.iso"))
    if not candidates:
        raise SystemExit("ERROR: libyaul did not output a Saturn ISO image")
    iso = max(candidates, key=lambda path: path.stat().st_size)
    size = iso.stat().st_size
    if size % 2048:
        raise SystemExit("ERROR: ISO size not multiple of 2048")
    a.dest.mkdir(parents=True, exist_ok=True)
    target = a.dest / (a.name + ".bin")
    cue = a.dest / (a.name + ".cue")
    with iso.open("rb") as source:
        header = source.read(16)
        if header != b"SEGA SEGASATURN ":
            raise SystemExit("ERROR: ISO lacks Sega Saturn boot header")
        source.seek(16 * 2048 + 1)
        if source.read(5) != b"CD001":
            raise SystemExit("ERROR: ISO lacks ISO9660 primary volume descriptor")
        source.seek(0)
        with target.open("wb") as out:
            for lba in range(size // 2048):
                out.write(raw_mode1_sector(lba, source.read(2048)))
    cue.write_text(
        f'FILE "{target.name}" BINARY\n'
        '  TRACK 01 MODE1/2352\n'
        '    INDEX 01 00:00:00\n',
        encoding="ascii"
    )
    print(f"OK: {iso} ({size} bytes) -> {target} ({target.stat().st_size} bytes)")
    print(f"CUE: {cue}")

if __name__ == "__main__":
    main()
