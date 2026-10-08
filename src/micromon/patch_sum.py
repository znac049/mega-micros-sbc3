#!/usr/bin/env python3
"""Patch the checksum longword into a 4 MB 68k ROM image.

Usage: romsum.py rom.bin [offset]
  offset defaults to the last longword (0x3FFFFC). It must match where your
  ROM source reserves the slot, e.g. at the very end of the image:
        org     $FFFFFC
        dc.l    0               ; checksum, patched by romsum.py
"""
import struct, sys

ROM_BASE = 0xc00000
ROM_SIZE = 4 * 1024 * 1024

def ones_sum(data):
    s = 0
    for (w,) in struct.iter_unpack(">I", data):   # 68k is big-endian
        s += w
        s = (s & 0xFFFFFFFF) + (s >> 32)           # end-around carry
    return s



path = sys.argv[1]

if len(sys.argv) > 2:
    off = int(sys.argv[2], 0)
    if off > ROM_SIZE:
        off = off - ROM_BASE
else:
    off = ROM_SIZE - 4

rom = bytearray(open(path, "rb").read())
rom += b'\xff' * (ROM_SIZE - len(rom))

if len(rom) != ROM_SIZE:
    sys.exit(f"image is {len(rom)} bytes, expected {ROM_SIZE}")

if off % 4:
    sys.exit("checksum offset must be longword aligned")

rom[off:off+4] = b"\0\0\0\0"
csum = ~ones_sum(rom) & 0xFFFFFFFF
rom[off:off+4] = struct.pack(">I", csum)

assert ones_sum(rom) == 0xFFFFFFFF

open(path, "wb").write(rom)
print(f"checksum ${csum:08X} written at offset ${off:06X}")