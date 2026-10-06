"""Run the firmware command smoke test over native USB Serial/JTAG."""
import argparse
import time
from pathlib import Path
import serial
from check_smoke_log import check

parser = argparse.ArgumentParser()
parser.add_argument("--port", required=True)
parser.add_argument("--log", default="smoke.log")
parser.add_argument("--timeout", type=float, default=900)
args = parser.parse_args()
lines = []
with serial.Serial(args.port, 115200, timeout=0.2) as port:
    port.write(b"test\n")
    deadline = time.monotonic() + args.timeout
    while time.monotonic() < deadline:
        text = port.readline().decode("utf-8", errors="replace")
        if not text:
            continue
        print(text, end="", flush=True)
        lines.append(text)
        if "COMPLETE: pass=" in text or "CANCELLED:" in text:
            break
    else:
        port.write(b"stop\n")
        print("Timed out; cancellation requested.")
log = "".join(lines)
Path(args.log).write_text(log, encoding="utf-8")
message, code = check(log)
print(message)
raise SystemExit(code)
