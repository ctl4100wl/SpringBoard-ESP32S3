# Windows mouse control

The firmware compiles the original `src/mouse_cursor_icon.c` resource and shows
it on the display after the first valid mouse packet. Its tip follows native
coordinates; button release/disconnection does not hide the pointer.

Run `artifacts/SpringBoard-Pointer-Bridge.exe` in the PlatformIO project, enter the device's COM port (currently COM6), then click **Connect**. Close PlatformIO monitor or another serial program first; Windows allows one owner of the port.

The dark rectangle is a 4× enlarged 128×160 control pad. Watch the actual ESP32 screen while moving the mouse over this pad. Hold the left mouse button to touch, drag, scroll or perform navigation gestures. Right-click sends `back`. The buttons open Home, recents, Control Center, the notification shade, and start/stop tests. The text field sends any supported serial command, including `type TEXT` for a focused text field. This application does not mirror the display.

Motion updates are coalesced and limited to approximately 60 Hz. Touch edges are queued separately. A packet contains five bytes:

| Byte | Meaning |
| --- | --- |
| 0 | `0xA5` sync |
| 1 | Touch pressed: 0 or 1 |
| 2 | Physical X: 0–127 |
| 3 | Physical Y: 0–159 |
| 4 | XOR of bytes 0–3 |

The firmware validates checksums and coordinates, coalesces motion, and retains button edges for LVGL's pointer reader. No log message is emitted per mouse packet. A pressed pointer releases after two seconds without a valid packet. ASCII newline-terminated commands continue to work on the same connection. `status` reports accepted pointer packets and UI state.

The protocol is for the present portrait panel; a future 320×240 panel needs a wider coordinate packet and updated display configuration.

Build the dependency-free native Windows EXE with the installed MinGW compiler:

```powershell
C:/msys64/ucrt64/bin/gcc.exe -O2 -Wall -Wextra -Werror -Wno-misleading-indentation -static -mwindows pointer_bridge.c -o SpringBoard-Pointer-Bridge.exe -luser32 -lgdi32
```

Serial transport runs on a worker thread, keeping the control window responsive during display rendering. On disconnect or close, the application sends a release packet. Connect does not assert RTS/DTR, avoiding intentional device resets.
