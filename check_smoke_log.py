"""Check an actual serial smoke-test log; exits nonzero for failures/incomplete runs."""
import re
import sys
from pathlib import Path

def check(text):
    # Analyze the latest requested run, not an earlier successful run in the same log.
    start = text.rfind("START three app/CC/notification/physics cycles")
    if start < 0:
        return "FAIL: no smoke-test start found", 1
    run = text[start:]
    summary = re.search(r"COMPLETE: pass=(\d+) fail=(\d+) capability_skips=(\d+)", run)
    if not summary:
        return "FAIL: latest run did not complete (cancelled, crashed or still running)", 1
    passes, fails, skips = map(int, summary.groups())
    cycles = re.findall(r"Cycle (\d+) settled heap=(\d+)", run)
    if fails or re.search(r"\bFAIL\b|Guru Meditation|assert failed|abort\(\)", run):
        return f"FAIL: {fails} failed checks; inspect serial output", 1
    if [int(cycle) for cycle, _ in cycles] != [1, 2, 3]:
        return "FAIL: missing three settled cycle reports", 1
    heaps = ", ".join(heap for _, heap in cycles)
    return f"PASS: {passes} smoke checks; {skips} capability skips; cycle heaps: {heaps}. Hardware/layout QA still required.", 0

if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: python check_smoke_log.py SERIAL_LOG")
    message, code = check(Path(sys.argv[1]).read_text(encoding="utf-8", errors="replace"))
    print(message)
    raise SystemExit(code)
