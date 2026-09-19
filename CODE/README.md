# DIY Game Controller Firmware

C firmware for the RP2040-based Xbox-style game controller, built for the
Raspberry Pi Pico VS Code extension.

## File Structure

```
firmware/
├── CMakeLists.txt           Build configuration (VS Code extension compatible)
├── pico_sdk_import.cmake    Standard SDK locator script
├── include/                 Public headers (one per subsystem)
│   ├── config.h             GPIO pin map + tunable constants — edit this first
│   ├── hid.h                USB HID report descriptor/struct
│   ├── inputs.h              Button + thumbstick polling
│   ├── triggers.h           AS5600 I2C trigger driver
│   ├── leds.h                WS2812B PIO LED driver
│   ├── deadzone.h           Deadzone + response curve math
│   └── config_store.h       Flash-persisted settings (littlefs)
├── src/
│   ├── main.c                Entry point, main 1 kHz loop
│   ├── hid.c, inputs.c, triggers.c, leds.c, deadzone.c, config_store.c
│   ├── usb_descriptors.c    TinyUSB device/config/string descriptors
│   ├── tusb_config.h        TinyUSB compile-time config
│   └── ws2812.pio            PIO assembly for WS2812B timing
└── .vscode/                  Extension workspace settings
```

## Prerequisites

1. **VS Code** with the **Raspberry Pi Pico** extension installed
   (search "Raspberry Pi Pico" in the Extensions marketplace).
   The extension manages the SDK, ARM toolchain, CMake, and Ninja for you —
   no manual toolchain install needed.

2. **pico-littlefs** (required third-party dependency, NOT part of the
   official Pico SDK). This project's flash-based settings storage
   (`config_store.c`) depends on it. Add it as a submodule from the
   firmware project root:

   ```bash
   git init                     # if this folder isn't already a git repo
   git submodule add https://github.com/lurk101/pico-littlefs.git lib/pico-littlefs
   git submodule update --init --recursive
   ```

   If you skip this step, `CMakeLists.txt` will print a warning during
   configuration and `config_store.c` will fail to compile.

## Building

1. Open this folder in VS Code (`File > Open Folder...`).
2. The Pico extension should detect `CMakeLists.txt` and prompt to configure
   — accept it, or run **Raspberry Pi Pico: Configure CMake** from the
   command palette (Ctrl+Shift+P).
3. Click **Compile** in the Pico extension's sidebar, or run
   **Raspberry Pi Pico: Compile Project**.
4. The build produces `build/game_controller.uf2`.

## Flashing

1. Hold the **BOOTSEL** button on the Pico while plugging in USB.
2. The Pico mounts as a USB drive named `RPI-RP2`.
3. Drag `build/game_controller.uf2` onto that drive — the Pico reboots
   automatically running the new firmware.

Alternatively, use the Pico extension's **Run** button (with a Debug Probe
connected via SWD) for one-click flash + debug.

## Hardware Notes

- All pin assignments are centralised in `include/config.h` — if your wiring
  differs from the KiCad schematic, edit pin numbers there only.
- The right thumbstick (`PIN_RSTICK_X`/`PIN_RSTICK_Y`, GP10/GP11) is **not**
  wired to native RP2040 ADC pins. See the comment block in `config.h` for
  options (external ADC IC, or rewire to GP28/GP29).
- Both AS5600 trigger sensors share I2C address `0x36` and **must** be on
  separate I2C buses (I2C0 for left, I2C1 for right) — this is already
  reflected in the pin map and schematic.

## Firmware Behaviour Summary

- Polls all buttons/sticks/triggers at approximately 1 kHz.
- Sends one USB HID report per cycle (capped at 125 Hz by USB Full-Speed
  HID's 8 ms minimum interrupt interval — see `HID_REPORT_INTERVAL_MS`).
- Applies a circular deadzone + selectable response curve (linear /
  quadratic / S-curve) to each thumbstick.
- Maps each trigger's raw AS5600 angle into 0–32767 using stored
  calibration values.
- Runs a WS2812B idle/active/rainbow LED animation at ~60 fps.
- Holding Menu+View+Guide for 3 seconds is detected as the remap-mode
  entry gesture (the interactive remap UI itself is left as a follow-up —
  see the `TODO` comment in `main.c`).
- Settings (deadzones, curves, trigger calibration, button remap, turbo,
  LED mode) persist across power cycles via littlefs in flash.
