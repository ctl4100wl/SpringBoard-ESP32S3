"""Compare actual completed DMA frame rates on the attached SpringBoard device."""
import argparse
import time
from pathlib import Path
import serial

parser = argparse.ArgumentParser()
parser.add_argument("--port", required=True)
parser.add_argument("--log", default="perf.log")
parser.add_argument("--seconds", type=float, default=11)
args = parser.parse_args()
if args.seconds < 6:
    parser.error("--seconds must be at least 6 for a complete reporting window")
cases = [
    ("native plain direct", "effects test plain\nrender direct"),
    ("native plain full", "render full"),
    ("native blur full", "effects test blur\nrender full"),
    ("native blur direct", "render direct"),
    ("native lens full", "effects test lens\nrender full"),
    ("native lens direct", "render direct"),
]
with serial.Serial(args.port, 115200, timeout=0.1) as port, Path(args.log).open("w", encoding="utf-8") as log:
    port.write(b"stop\ncc close\nshade close\nunlock\nhome\n")
    time.sleep(2)
    port.reset_input_buffer()
    port.write(b"perf on\n")
    try:
        for name, command in cases:
            heading = f"\nCASE {name}\n"
            print(heading, end="", flush=True)
            log.write(heading)
            port.write((command + "\nperf reset\n").encode())
            deadline = time.monotonic() + args.seconds
            while time.monotonic() < deadline:
                text = port.read(8192).decode("utf-8", errors="replace")
                print(text, end="", flush=True)
                log.write(text)
                log.flush()
                if "Guru Meditation" in text or "assert failed" in text:
                    raise RuntimeError("Firmware fault; inspect the saved log")
    finally:
        port.write(b"effects test off\nrender auto\nperf off\nhome\n")
