"""Build both blur defaults from one PlatformIO configuration and package each ELF/BIN."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--project', type=Path, required=True)
parser.add_argument('--pio', default='platformio')
parser.add_argument('--env', default='4d_systems_esp32s3_gen4_r8n16')
parser.add_argument('--esptool', type=Path, required=True)
parser.add_argument('--wad', type=Path)
parser.add_argument('--jobs', type=int, default=8)
args = parser.parse_args()
project = args.project.resolve()
output = project / 'artifacts'
output.mkdir(exist_ok=True)
variant_header = project / 'src/port/render_variant.h'
build = project / '.pio/build' / args.env
records = {}
for name, enabled in (('NoBlur', 0), ('Blur', 1)):
    variant_header.write_text('#pragma once\n#define S3_DEFAULT_BLUR %d\n' % enabled)
    subprocess.run([args.pio, 'run', '--project-dir', str(project), '--environment', args.env, '--jobs', str(args.jobs)], check=True)
    stem = 'SpringBoard-' + name + '-40MHz'
    for source, suffix in (('firmware.bin', '-app.bin'), ('firmware.elf', '.elf')):
        shutil.copy2(build / source, output / (stem + suffix))
    for source in ('bootloader.bin', 'partitions.bin'):
        shutil.copy2(build / source, output / source)
    subprocess.run([sys.executable, str(args.esptool), '--chip', 'esp32s3', 'merge_bin', '-o', str(output / (stem + '-full.bin')),
                    '--flash_mode', 'dio', '--flash_freq', '80m', '--flash_size', '16MB',
                    '0x0', str(build / 'bootloader.bin'), '0x8000', str(build / 'partitions.bin'), '0x10000', str(build / 'firmware.bin')], check=True)
    records[name] = {'default_blur': bool(enabled), 'lcd_spi_hz': 40000000,
                     'files': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in output.glob(stem + '*')}}
if args.wad:
    data = args.wad.read_bytes()
    if data[:4] != b'IWAD' or len(data) > 0x500000:
        raise ValueError('WAD must be an IWAD and fit the 5 MiB partition')
    shutil.copy2(args.wad, output / 'DOOM1.WAD')
    records['wad'] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(), 'flash_offset': '0x500000'}
(output / 'renderer-builds.json').write_text(json.dumps(records, indent=2))
print(output)
