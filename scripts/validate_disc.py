#!/usr/bin/env python3
"""Validate Saturn raw CD data and BIN/CUE linkage."""
import argparse
from pathlib import Path

def main():
    p = argparse.ArgumentParser()
    p.add_argument("bin", type=Path)
    p.add_argument("cue", type=Path)
    a = p.parse_args()
    size = a.bin.stat().st_size
    if size % 2352 or size < 20 * 2352:
        raise SystemExit("Invalid size: BIN must contain >=20 raw 2352-byte sectors")
    cue = a.cue.read_text(encoding="ascii")
    if f'FILE "{a.bin.name}" BINARY' not in cue or "TRACK 01 MODE1/2352" not in cue:
        raise SystemExit("Invalid CUE: expected raw MODE1/2352 matching BIN name")
    with a.bin.open("rb") as f:
        first = f.read(2352)
        f.seek(16 * 2352)
        pvd = f.read(2352)
    if first[:12] != b"\x00" + b"\xff"*10 + b"\x00":
        raise SystemExit("Invalid CD sync")
    if first[12:16] != bytes((0x00, 0x02, 0x00, 0x01)):
        raise SystemExit("Incorrect MODE1 first-sector time/mode")
    if first[16:32] != b"SEGA SEGASATURN ":
        raise SystemExit("Sega Saturn boot identifier missing")
    if pvd[16:23] != b"\x01CD001\x01":
        raise SystemExit("ISO9660 PVD at LBA16 missing")
    print(f"PASS: Saturn boot, ISO9660 volume, CUE; {size // 2352} sectors")
    print("NOTE: structural verification only; no Saturn emulator run was performed.")
if __name__ == "__main__":
    main()
