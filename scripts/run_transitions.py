"""Profile original app morphs and system panels on the physical display."""
import argparse
from pathlib import Path
import time
import serial

parser = argparse.ArgumentParser()
parser.add_argument('--port', default='COM6')
parser.add_argument('--effects', choices=('off', 'blur'), default='off')
parser.add_argument('--log', default='transitions.log')
args = parser.parse_args()
output = []
with serial.Serial(args.port, 115200, timeout=0.05) as port:
    def command(text, seconds):
        port.write((text + '\n').encode())
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            line = port.readline().decode(errors='replace')
            if line:
                print(line, end='', flush=True)
                output.append(line)
    command('stop\ncc close\nshade close\nunlock\nhome\nperf off\nrender auto\neffects ' + args.effects, 3)
    for cycle in range(3):
        for app in (0, 1, 2, 3):
            command(f'profile on\nperf reset\napp {app}', 0.8)
            command('perf\nprofile off', 0.2)
            command('profile on\nperf reset\nhome', 0.8)
            command('perf\nprofile off', 0.2)
        for action in ('cc open', 'cc close', 'shade open', 'shade close', 'recents pull', 'home'):
            command('profile on\nperf reset\n' + action, 1.1)
            command('perf\nprofile off', 0.2)
    command('effects ' + args.effects + '\nrender auto\nstatus', 0.5)
Path(args.log).write_text(''.join(output), encoding='utf-8')
