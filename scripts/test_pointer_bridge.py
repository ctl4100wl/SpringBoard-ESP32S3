"""Hardware check for compact mouse packets and real app hit testing."""
import argparse
import re
import time
from pathlib import Path
import serial

def packet(pressed, x, y):
    return bytes((0xA5, pressed, x, y, 0xA5 ^ pressed ^ x ^ y))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', default='COM6')
    parser.add_argument('--log', default='pointer-test.log')
    args = parser.parse_args()
    transcript = []
    with serial.Serial(args.port, 115200, timeout=0.1) as port:
        def command(text, delay=0.15):
            port.write((text+'\n').encode())
            time.sleep(delay)
            result = port.read(32768).decode(errors='replace')
            transcript.append(result)
            return result
        command('stop\ncc close\nshade close\nunlock\nhome\nrender auto', 3)
        before = command('status')
        count = int(re.findall(r'pointer_packets=(\d+)', before)[-1])
        broken = bytearray(packet(0, 19, 25))
        broken[4] ^= 1
        port.write(broken + packet(0, 128, 25))
        after = command('status')
        assert int(re.findall(r'pointer_packets=(\d+)', after)[-1]) == count, 'Invalid packet was accepted'
        port.write(packet(0, 19, 25))
        time.sleep(0.1)
        port.write(packet(1, 19, 25))
        time.sleep(0.15)
        held = command('status')
        assert 'x=19 y=25 pressed=1' in held, 'Pointer coordinate/button state mismatch'
        port.write(packet(0, 19, 25))
        time.sleep(1.5)
        settled = command('status')
        assert 'app=1 busy=0' in settled, 'Native pointer did not launch the Home icon'
        assert 'pressed=0' in settled, 'Release edge was lost'
        port.write(packet(1, 127, 159))
        time.sleep(2.3)
        released = command('status')
        assert 'pressed=0' in released, 'Stale pointer did not release'
        command('home', 1)
    transcript.append('PASS checksum/bounds/button edges/native hit testing/stale release\n')
    Path(args.log).write_text(''.join(transcript), encoding='utf-8')
    print(transcript[-1], end='')

if __name__ == '__main__':
    main()
