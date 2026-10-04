# Cyber Pet

A pixel-art fairy (perhaps a version of yourself?) that lives on a small TFT screen and reacts to the real weather. An ESP32 connects to WiFi, fetches the current weather and time, and plays the matching animation in front of a weather backdrop (sun, clouds, rain, snow, or a night sky).

<img width="2048" height="1536" alt="WhatsApp Image 2026-10-05 at 00 57 21" src="https://github.com/user-attachments/assets/8a50ec80-5adf-4b55-8a53-dc6ed27a9b2f" />

## Features

- Weather-aware animations: sunny, cloudy, rainy, snowy
- Animated backdrops drawn in code (rotating sun, drifting clouds, rain, snow, twinkling stars and moon)
- Clock (top left) and temperature (top right)
- Weather refreshed every 15 minutes, no API key needed (Open-Meteo)
- Flicker-free drawing using an off-screen canvas

## Hardware

- ESP32 dev board (38-pin, USB-C, CP2102 USB chip)
- 1.8" 128x160 SPI TFT display (ST7735)
- USB cable that carries data (not charge-only)

### Wiring (display to ESP32)

All connections at 3.3V, not 5V.

| Display pin | ESP32 pin |
|---|---|
| VCC | 3V3 |
| GND | GND |
| CS | GPIO 5 |
| RESET | GPIO 4 |
| A0 (DC) | GPIO 16 |
| SDA (MOSI) | GPIO 23 |
| SCK | GPIO 18 |
| LED (backlight) | 3V3 |

## Software setup

1. Install [VS Code](https://code.visualstudio.com/) and the **PlatformIO IDE** extension.
2. Install the CP210x USB driver if Windows doesn't show a COM port for the board.
3. Install Python 3 and Pillow (`pip install pillow`) for the art converter.
4. Open this folder in VS Code.
5. Create `src/secrets.h` from the example file and enter your WiFi details (see below).
6. Build and upload with PlatformIO.

The ESP32 only supports **2.4 GHz** WiFi. A 5 GHz-only network will not work.

### WiFi credentials

Copy `src/secrets.h.example` to `src/secrets.h` and fill it in. `secrets.h` is listed in `.gitignore`, so it is never committed.

```cpp
#pragma once
#define WIFI_SSID "your-wifi-name"
#define WIFI_PASS "your-wifi-password"
```

## Configuration

At the top of `src/main.cpp`:

| Setting | Meaning |
|---|---|
| `LAT`, `LON` | Location used for the weather |
| `TEST_CODE` | `-1` for real weather, or a test code: 0 sun, 3 cloud, 61 rain, 71 snow |
| `TEST_DAY` | `-1` for real time, `1` force day, `0` force night |

Display variant: if the screen shows a stray colored line at an edge, try a different tab setting in `platformio.ini` (`-DST7735_GREENTAB2` works for my screen; others are `REDTAB`, `BLACKTAB`, `GREENTAB`, `GREENTAB3`).

## Adding or changing art

1. Draw sprites in [Piskel](https://www.piskelapp.com/) on a **32x32** canvas with a **transparent** background.
2. Export each animation as PNG frames named `name_0.png`, `name_1.png`, ... and put them in `art/`.
3. Run `python convert.py`. It generates `src/frames.h` (frames scaled 3x to 96x96, stored in RGB565).
4. Upload to the board.

Animation speed per animation is set in the `DELAYS` table in `convert.py`. Keep editable `.piskel` source files in `art_source/`.

## How weather maps to animations

| Weather (WMO code) | Animation | Backdrop |
|---|---|---|
| Clear, day (0-1) | sunny | sun and grass |
| Clear, night | cloudy (no night animation yet) | stars and moon |
| Cloudy, fog (2, 3, 45, 48) | cloudy | gray clouds |
| Drizzle, rain, thunderstorm (51-67, 80-82, 95+) | rainy | dark clouds and rain |
| Snow (71-77, 85-86) | snowy | falling snow |

## Troubleshooting

- **Red underline on `#include <TFT_eSPI.h>`:** run a build once, then "PlatformIO: Rebuild IntelliSense Index".
- **Upload says no port found:** check the cable, install the CP210x driver, or set `upload_port` in `platformio.ini`.
- **"Failed to connect" on upload:** hold the BOOT button when the upload starts.
- **Stuck on "Connecting WiFi...":** network must be 2.4 GHz; check name and password.
- **Black box around the fairy:** make sure PNG backgrounds are transparent, run `python convert.py`, then upload again.


## Credits

- Weather data: [Open-Meteo](https://open-meteo.com/)
- Display library: [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- JSON parsing: [ArduinoJson](https://arduinojson.org/)
- Sprites drawn in Piskel
