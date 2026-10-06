"""Read the native RGB565 framebuffer and save a PNG without image dependencies."""
import argparse
from pathlib import Path
import struct
import time
import zlib
import serial

def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))

def png(pixels, width, height):
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        for x in range(width):
            c = struct.unpack_from('<H', pixels, 2*(y*width+x))[0]
            r, g, b = (c >> 11) & 31, (c >> 5) & 63, c & 31
            rows.extend(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)))
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b'')

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', default='COM6')
    parser.add_argument('--output', default='screen.png')
    parser.add_argument('--command', default='')
    args = parser.parse_args()
    with serial.Serial(args.port, 115200, timeout=0.2) as port:
        port.write(b'perf off\nstop\n')
        if args.command:
            port.write((args.command+'\n').encode())
        time.sleep(3)
        port.reset_input_buffer()
        port.write(b'screen\n')
        deadline = time.monotonic()+10
        while time.monotonic() < deadline:
            line = port.readline()
            if line.startswith(b'S3FRAME '):
                _, width, height, count = line.split()
                pixels = bytearray()
                transfer_end = time.monotonic()+10
                while len(pixels) < int(count) and time.monotonic() < transfer_end:
                    pixels.extend(port.read(int(count)-len(pixels)))
                if len(pixels) != int(count):
                    raise RuntimeError(f'Truncated framebuffer: {len(pixels)} / {int(count)} bytes')
                Path(args.output).write_bytes(png(pixels, int(width), int(height)))
                print(args.output)
                return
        raise RuntimeError('No framebuffer header from firmware')

if __name__ == '__main__':
    main()
