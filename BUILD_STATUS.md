# ESP32-S3 renderer, browser and Doom verification — 2026-10-06

Actual PlatformIO project and original shell remain as documented below.
Two release defaults are packaged as `SpringBoard-NoBlur-40MHz` and
`SpringBoard-Blur-40MHz`, each with app BIN, merged full BIN and debug ELF.
The LCD stays at 40 MHz with two internal DMA buffers and idle dirty refresh.

## Current hardware checks

- Boot and flash hashes verified on COM6 after adding Doom, QuickJS and ThorVG.
- RGB565 SIMD alignment/guard checks and ARGB SIMD alignment/tail checks pass.
- Actual ThorVG canvas integration passes: pixel `ff22cc55`, 56 SIMD spans.
- Original pointer image is compiled and visible at the binary bridge coordinates.
- Calculator fits its keypad; Clock tab bar scales correctly; Calendar fits its
  month and details card. Native framebuffer screenshots were inspected.
- Browser `about:js` runs arrow functions, const, map and document.write through
  QuickJS on the MCU. Existing about:home content is unchanged.
- Wi-Fi station initialization and asynchronous scan pass: nine nearby networks.
  Settings has network/password fields and Join. No network was provisioned,
  so authentication, live HTTP/HTTPS and NTP remain unverified.
- Local DOOM1.WAD flash hash verified. Engine title screen and a playable level
  were captured; serial movement/fire controls worked. Audio and saves are absent.
- Static internal RAM fell from 201,076 to 110,292 bytes by moving Doom tables,
  engine BSS and browser scratch state into PSRAM. Runtime radio/render allocations
  are additional; initial linker RAM totals do not establish free DMA capacity.

Command smoke test: **169 passed, 0 failed, 12 capability skips** across three
cycles. The remaining skips are unsupported media/weather services. The final
Doom worker uses an external PSRAM stack to preserve internal RAM for Wi-Fi.
Flash mapping runs on the internal UI stack, as required by ESP-IDF. A combined
boot passed radio scanning (10 networks), QuickJS and entering a playable Doom
level without reset; internal free heap was 34,299 bytes. Settings Wi-Fi fields
and Join button were inspected on the native display.

## Real original transition measurements

Three cycles of app launches/closes (Settings, Calculator, Clock, Calendar),
Control Center, notification shade and Home recents gesture. Motion measurements
count completed display intervals while animations are active, excluding settled
idle time. Draw profiling was enabled, so these are diagnostic measurements.

| Default/effects | Nonzero motion samples | Range | Mean across samples |
| --- | ---: | ---: | ---: |
| No blur | 42 | 13.14–29.59 FPS | 18.16 FPS |
| Blur | 42 | 11.62–27.34 FPS | 17.08 FPS |

**Real transitions do not meet the requested 60 FPS target.** The older light
fixture results below remain fixture-only measurements. Renderer O3, a hot RGB565
fill in IRAM, LX7 fills and cached LCD viewport commands preserve shell geometry
and spring behavior; text, compositing and transformed layers still dominate.
Logs: `transitions-off.log`, `transitions-blur.log`, `features.log`, `doom-live.log`.

## Firmware and WAD

Use `artifacts/flash_renderer.cmd NoBlur COM6` or `Blur COM6`. The script writes
bootloader at 0, partitions at 0x8000, app at 0x10000 and DOOM1.WAD at 0x500000;
it preserves NVS. The full BIN includes the NVS gap and does not embed the WAD.
The WAD is copied only into local artifacts/backups, not committed to Git.
`configure_wifi.py` asks for credentials locally; serial password command logging
is redacted. Close the bridge/serial monitor before running flash/test tools.

---

## Previous release verification (historical)

# Native ESP32-S3 build verification â€” 2026-10-05

Actual project: `C:/Users/Admin/Documents/PlatformIO/Projects/SpringBoard`.
Branch: `codex/esp32-s3-feasibility`. PlatformIO Espressif32 7.0.1, ESP-IDF 6.0.1.
Attached ESP32-S3 revision 0.2, 8 MB PSRAM, native USB Serial/JTAG on COM6.

## Rendering and hardware

- Native **128 x 160 portrait** rasterization: no 384 x 480 screen buffers or downsampling.
- Original shell/apps, morphs, spring motion, widgets, folders, recents and panels retained.
- Native fonts, hand vertices, draw primitives, canvas dimensions and layout geometry.
- Two 40,960-byte internal DMA render buffers. Automatic full buffers during motion;
  direct rendering and dirty-row DMA when settled. No framebuffer copy to a staging buffer.
- 40 MHz LCD SPI with DMA; MOSI 11, MISO 13, SCLK 12, CS 10, DC 9, reset 8. No GPIO20.
- CPU 240 MHz, PSRAM 40 MHz, -O2, 32 KB instruction cache, 64 KB data cache/64-byte lines.
- LX7 vector fill/copy assembly, guarded ABI adapter and C fallbacks.
- Opaque top/system overlays skip hidden screen redraws using LVGL coverage checks.
- Runtime blur/lens default off; comparative fixtures exercise both original renderers.
- Native Latin/Vietnamese/symbol fonts included. Font and SIMD licenses are shipped.

Static RAM: 135,088 / 327,680 bytes (41.2%). Program: 1,473,523 / 2,031,616
bytes (72.5%). App BIN: 1,473,984 bytes; merged BIN: 1,539,520 bytes;
ELF: 16,108,816 bytes. Runtime DMA buffers/layers are additional allocations.
An isolated GCC internal crash cleared when rebuilt with `--jobs 2`;
`artifacts/build-native.log` records the successful final build.

## Measured completed display FPS

Moving full-screen striped fixture with a glass pane; five-second reporting windows.
These are completed DMA frames, not invalidation counts. They are not a guarantee
that every app or transition sustains the same rate.

| Scene | Direct buffers | Full buffers |
| --- | ---: | ---: |
| Plain | 61.27 FPS | 61.57 FPS |
| Blur | 58.93 FPS | 61.02 FPS |
| Lens with chromatic fringe | 54.80 FPS | 59.59 FPS |

Full-frame byte swap: about 0.64 ms; SPI transfer: about 8.47â€“8.50 ms.
Full-mode UI times: plain 4.56 ms, blur 8.21 ms, lens 9.47 ms.
Before native rendering, the same display's best large-frame comparison was about
8.77 FPS. Logs: `artifacts/perf-native.log`, `perf-native-first.log`, and the earlier
`hardware-perf-final.log`. The final recents fix changes command bookkeeping only.

## Verification

- Build and flash hash verified on COM6; NVS preserved during app-only updates.
- BIN checksum and embedded validation hash checked with esptool.
- Merged app region exactly matches app BIN; merged bootloader/partitions and aliases checked.
- Actual PlatformIO hardware and OS sources match the canonical inputs.
- Device screenshots reviewed: Home, clock hands, lock-clock spacing and Control Center.
- Compact pointer protocol passed invalid checksum/bounds, press/release, native icon hit testing,
  and two-second stale-input release checks. Log: `artifacts/pointer-native.log`.
- Cancellation passed over `stop`, the same cancellation path invoked by BOOT.
  Physical second-press debounce is implemented; it was not independently actuated by the host.
- SIMD fill/copy/guards/unaligned fallback passed on the device; actual LX7 vector instructions
  are present. The Windows bridge compiled with warnings treated as errors and no MSYS runtime dependency.
- Windows bridge GUI interaction itself was not automated; its device protocol was tested.

Final hardware smoke gate: **169 passed, 0 failed, 18 capability skips**, all three
cycles complete. It includes app launch/close morphs, Control Center, notification
fade/reflow, spring retarget/kick/opacity, real Home recents pull-up, all eight
snapshot-card removals per cycle, and blur/lens rendering. The serial recents helper
now selects the actual snapshot and updates MRU through the original swipe path.
Settled cycle internal heaps: 64,447 / 64,291 / 64,407 bytes; initial 64,219,
final 64,407. Minimum observed 60,991 bytes. Log: `artifacts/smoke-native.log`.
The final firmware's smoke scenes measured blur 60.97 FPS and lens 59.91 FPS.

Network/media/Doom capability checks are explicitly skipped beyond UI creation.
This port does not provision Wi-Fi or prove those desktop services function on the MCU.

## Artifacts

`SpringBoard-Native-40MHz-app.bin`, `SpringBoard-Native-40MHz-full.bin`,
`SpringBoard-Native-40MHz.elf`, matching `SpringBoard-*` aliases,
`SpringBoard-Pointer-Bridge.exe`, `bootloader.bin`, `partitions.bin`,
`SHA256SUMS.txt`, scripts and hardware logs in the actual project's `artifacts` folder.

Use app BIN at 0x10000 for an existing installation. ELF contains debugging symbols.
The merged full BIN fills the NVS gap; see `FLASH.txt` for preserving settings.
The original OG 40 MHz rollback artifacts remain available.
