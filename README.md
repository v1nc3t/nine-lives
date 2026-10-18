# nine lives

Firmware for the M5Stack Cardputer (ESP32-S3), written in C++ with PlatformIO and the Arduino framework. See [PLAN.md](PLAN.md) for the roadmap.

## Setup

1. Install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/) (`pipx install platformio` or `pip install platformio`), or the PlatformIO IDE extension for VS Code.
2. Clone the repo. The first `pio run` downloads everything else (ESP32 platform, toolchain, and the `M5Cardputer` library with its dependencies `M5Unified`, `M5GFX` and `IRremote`).
3. On Linux, your user needs serial access: add yourself to the `dialout` group (`sudo usermod -aG dialout $USER`), then log out and back in.

## Build and flash

```bash
pio run                  # compile
pio run -t upload        # compile and flash to the Cardputer
pio device monitor       # serial output (115200 baud, Ctrl+C to exit)
pio run -t upload && pio device monitor   # flash, then watch output
```

To flash:

1. Connect the Cardputer to your computer with a USB-C cable.
2. If it is not detected, or the upload fails, put it in download mode: hold the **G0** button (side of the device), tap reset (or plug in the cable while holding G0), then release G0.
3. Run `pio device list` to check that a serial port appears (for example `/dev/ttyACM0`), then run `pio run -t upload`.
4. Press reset to run the new firmware. After a flash in download mode you may need to replug the cable.

## Writing code

Code lives in `src/`; `src/main.cpp` is the entry point. Everything comes from one include:

```cpp
#include <M5Cardputer.h>

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);   // true = enable the keyboard
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.drawString("Hello", 10, 10);
}

void loop() {
    M5Cardputer.update();           // call every loop to read the keyboard
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        auto state = M5Cardputer.Keyboard.keysState();
        for (char c : state.word) Serial.print(c);
    }
}
```

Useful objects: `M5Cardputer.Display` (screen), `M5Cardputer.Keyboard`, `M5Cardputer.Speaker`, `M5Cardputer.Mic`, `M5Cardputer.Power`. Library docs and examples: <https://github.com/m5stack/M5Cardputer>.

Libraries are declared in `platformio.ini` under `lib_deps`; add more there and PlatformIO fetches them on the next build.

## Editor setup (clangd)

If your editor shows errors such as `Use of undeclared identifier 'M5Cardputer'` even though `pio run` works, generate the compilation database and restart the language server:

```bash
pio run -t compiledb     # writes compile_commands.json (gitignored)
```

Re-run it after changing `lib_deps`.

The repo includes the rest of the clangd setup: `.clangd` hides GCC-only flags clang does not know, and `.zed/settings.json` starts clangd with `--query-driver=**/xtensa-esp32s3-elf-*` so it finds the Xtensa toolchain's system headers (otherwise: `'machine/endian.h' file not found`, followed by bogus errors such as `no member named 'drawString'`). In other editors, pass the same `--query-driver` flag to clangd.

## Troubleshooting

- **No serial port shown:** use a data USB-C cable (not charge-only) and try download mode (hold G0 while connecting).
- **Permission denied on `/dev/ttyACM0`:** you are not in the `dialout` group (see Setup).
- **Serial monitor is empty:** the Cardputer's USB port re-enumerates after reset, so rerun `pio device monitor`.
- **Upload works but nothing runs:** press the reset button once.
