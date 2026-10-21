#!/usr/bin/env python3
"""Inject only the user's 512 KiB .md ROM into an already compiled Saturn BIN.

The public GitHub repository never includes commercial ROM data.
Run this locally; do not upload the resulting disc to a public repo.
"""
import argparse
from hashlib import sha256
from pathlib import Path
from iso_to_bin_cue import raw_mode1_sector

MARKER = b"MEGAORBIT-SONIC-SLOT-V1".ljust(32, b"\x00")
SIZE = 524288
BLANK = 0xE7

def patch_disc(source, game, output):
    rom = game.read_bytes()
    if len(rom) != SIZE:
        raise ValueError(f"Expected 512 KiB ROM, found {len(rom)} bytes")
    if rom[0x100:0x10f] != b"SEGA MEGA DRIVE":
        raise ValueError("Not a raw Mega Drive ROM with SEGA header")
    raw = source.read_bytes()
    if len(raw) % 2352:
        raise ValueError("Not a raw MODE1/2352 BIN")
    sector_count = len(raw)//2352
    if raw[16:32] != b"SEGA SEGASATURN ":
        raise ValueError("Missing Saturn boot header")
    if raw[16*2352+16:16*2352+23] != b"\x01CD001\x01":
        raise ValueError("Missing ISO9660 primary volume descriptor")
    data = bytearray().join(raw[lba*2352+16:lba*2352+2064]
                            for lba in range(sector_count))
    pos = data.find(MARKER)
    if pos < 0 or data.find(MARKER, pos+1) >= 0:
        raise ValueError("Expected exactly one MegaOrbit private ROM slot")
    begin = pos+32
    if data[begin:begin+SIZE] != bytes([BLANK])*SIZE:
        raise ValueError("ROM placeholder corrupt or already replaced")
    data[begin:begin+SIZE] = rom
    first,last = begin//2048,(begin+SIZE-1)//2048
    result = bytearray(raw)
    for lba in range(first,last+1):
        original = result[lba*2352:(lba+1)*2352]
        rebuilt = raw_mode1_sector(lba,data[lba*2048:(lba+1)*2048])
        if rebuilt[:16] != original[:16]:
            raise ValueError("Unexpected sector header or CD layout")
        result[lba*2352:(lba+1)*2352] = rebuilt
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes(result)
    assert len(result)==len(raw)
    print("PASS: private ROM inserted; CD EDC/ECC regenerated")
    print("ROM SHA256:", sha256(rom).hexdigest())
    print("BIN:",output)

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--bin",required=True,type=Path)
    parser.add_argument("--rom",required=True,type=Path)
    parser.add_argument("--out",required=True,type=Path)
    args=parser.parse_args()
    patch_disc(args.bin,args.rom,args.out)
