#!/usr/bin/env python3

import sys
import getopt
import subprocess
from pathlib import Path

one_meg = 1024 * 1024

rom_size = 4 * one_meg
fs_size = 4 * one_meg

args = sys.argv[1:]
options = "m:i:s:"
long_options = ["monitor=", "image=", "size="]

monitorfile = 'micromon.part1'
romdisk = 'romdisk.bin'

try:
    args, values = getopt.getopt(args, options, long_options)

    for arg, val in args:
        if arg in ("-m", "--monitor"):
            monitorfile = val
        elif arg in ("-i", "--image"):
            romdisk = val
        elif arg in ("-s", "--size"):
            fs_size = one_meg * int(val)

except getopt.error as err:
    print(str(err))
    exit()


block_size = 2048

file = Path(monitorfile)
if not file.exists():
    print(f"Can't find file '{monitorfile}'")
    exit()

monitor_size = file.stat().st_size + 8  # 8 Bytes for the inbetween file (signature)
print(f"monitor size is {monitor_size}")

# Create a file to go between the monhitor with an easy to find signature
with open('inbetween.bin', mode="w+b") as f:
    f.write(b'\xde\xad\xfa\xce\xc0\x1d\xbe\xef')

free_rom_bytes = min(rom_size, fs_size) - monitor_size
print(f'Raw disk size is {free_rom_bytes}, (max={rom_size})')

rounded_block_count = int(free_rom_bytes / block_size)

with open(romdisk, mode="w+b") as f:
    f.write(b'\xff' * (block_size * rounded_block_count))

# cmd = ['dd', 'if=/dev/zero', f'of={romdisk}', f'bs={block_size}', f'count={rounded_block_count}']
cmd = ['mkfs.ext2', '-b', f'{block_size}', '-d', './romdisk', '-m', '1', '-M', '//rom', '-t', 'ext2', romdisk]
print(cmd)
try:
    res = subprocess.run(cmd, text=True, check=True)

    # print(cmd)

    # res = subprocess.run(cmd, text=True, check=True)
except subprocess.CalledProcessError as e:
    print(f'{cmd[0]} command failed with return code {e.returncode}')

