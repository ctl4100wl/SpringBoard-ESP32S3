"""Enter Wi-Fi credentials locally; the firmware does not echo the password."""
import argparse
import getpass
import time
import serial

parser = argparse.ArgumentParser()
parser.add_argument('--port', default='COM6')
args = parser.parse_args()
ssid = input('Network name: ')
password = getpass.getpass('Password (empty for open network): ')
if not ssid or len(ssid.encode()) > 32 or len(password.encode()) > 63 or any(c in ssid + password for c in '\r\n'):
    raise SystemExit('Invalid network name or password length.')
with serial.Serial(args.port, 115200, timeout=0.2) as port:
    port.write(('wifi ssid ' + ssid + '\nwifi password ' + password + '\nwifi connect\n').encode())
    deadline = time.monotonic() + 40
    while time.monotonic() < deadline:
        line = port.readline().decode(errors='replace')
        if line:
            print(line, end='')
        if 'Connected; IP=' in line:
            break
    else:
        raise SystemExit('Connection not confirmed. Check the credentials and try again.')
