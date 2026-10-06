"""PlatformIO pre-script: regenerate the ESP-IDF memory map after config changes.

PlatformIO can otherwise keep memory.ld from the previous instruction-cache size,
placing application code inside the new cache reservation and failing before app_main.
"""
from pathlib import Path

Import("env")  # supplied by SCons / PlatformIO
project = Path(env.subst("$PROJECT_DIR")).resolve()
build = Path(env.subst("$BUILD_DIR")).resolve()
expected = project / ".pio" / "build" / env.subst("$PIOENV")
if build != expected.resolve():
    raise RuntimeError(f"Unexpected build directory: {build}")
memory_map = build / "memory.ld"
if memory_map.is_file():
    memory_map.unlink()
