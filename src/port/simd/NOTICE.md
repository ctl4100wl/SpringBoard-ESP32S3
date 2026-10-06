# ESP32-S3 SIMD kernels

The three `.S` files are Apache-2.0 code copied unchanged from Espressif ESP-BSP, commit `c9a0f385cd1bf86e1bd07f14dc2cb77b86f20a9a`:
https://github.com/espressif/esp-bsp/tree/c9a0f385cd1bf86e1bd07f14dc2cb77b86f20a9a/components/esp_lvgl_port/src/lvgl9/simd

The local wrapper converts this project's newer LVGL descriptors into the assembly ABI explicitly. Only aligned opaque RGB565 fills/copies enter SIMD; all other operations retain LVGL's existing implementation. No ARM/PPA acceleration is claimed for LX7. `perf` reports SIMD invocation count.
