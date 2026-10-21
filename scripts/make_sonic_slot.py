#!/usr/bin/env python3
"""Create safe zero-content ROM placeholder used in CI builds.
The user's commercial ROM is inserted ONLY after downloading this public build.
"""
from pathlib import Path
SIGNATURE=b"MEGAORBIT-SONIC-SLOT-V1".ljust(32,b"\0")
ROM_SIZE=524288
path=Path("saturn/assets/sonic_slot.bin")
path.parent.mkdir(parents=True,exist_ok=True)
path.write_bytes(SIGNATURE+b"\xE7"*ROM_SIZE)
print("Created public no-ROM placeholder:",path,path.stat().st_size)
