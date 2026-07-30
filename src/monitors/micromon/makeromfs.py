#!/usr/bin/env python3

import subprocess
from pathlib import Path

filename = 'micromon.part1'
romdisk = 'romdisk.bin'

four_meg=1024*1024*4
block_size = 2048

file = Path(filename)
if not file.exists():
    print(f"Can't find file '{filename}'")
    exit()

monitor_size = file.stat().st_size

free_rom_bytes = four_meg - monitor_size
print(f'Raw disk size is {free_rom_bytes}, (max={four_meg})')

rounded_block_count = int(free_rom_bytes / block_size)

cmd = ['dd', 'if=/dev/zero', f'of={romdisk}', f'bs={block_size}', f'count={rounded_block_count}']
print(cmd)
try:
    res = subprocess.run(cmd, text=True, check=True)

    cmd = ['mkfs.ext2', '-b', f'{block_size}', '-m', '1', '-M', '//rom', '-t', 'ext2', romdisk]
    print(cmd)

    res = subprocess.run(cmd, text=True, check=True)
except subprocess.CalledProcessError as e:
    print(f'dd command failed with return code {e.returncode}')

