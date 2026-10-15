## PackOSLite - memory constrained version of PackOS for older AVR based Microcontrollers

PackOSLite is a lightweight, portrait-only version of PackOS, designed for Arduino-compatible AVR microcontrollers and a 240 × 320 ILI9341 SPI TFT display.
Display name used is "PackOS Lambda" as that's a cooler name for your backpack than "Lite"

The primary target was the **Adafruit Feather 32u4 Basic**, using an ATmega32U4 processor.

Other AVR boards may work with minor modifications, primarily to pin assignments and display wiring.

### 1. Hardware requirements

- Arduino-compatible AVR microcontroller
- Hardware SPI interface
- ILI9341-compatible 240 × 320 SPI TFT display
- At least 32 KB flash recommended
- At least 2 KB SRAM recommended
- Suitable power supply and logic-level compatibility

The touchscreen and SD card functions are not required.

### 2. Supported and potential boards

| Board / MCU | Status |
| Adafruit Feather 32u4 | Primary target |
| Arduino Leonardo / Micro | Expected to work |
| Arduino Uno / Nano (ATmega328P) | May work; memory constrained |
| ATmega1284P boards | Expected to work with a suitable board core |
| Arduino Nano Every (ATmega4809) | Requires compatibility testing |
| ATtiny85 | Not recommended |

The Feather 32u4 is the primary development target. Other boards have not been verified.

### 3. Configuring display pins

Open `PackOS_Lamda.ino` and locate the `Config` namespace near the beginning of the file.

The default configuration is:

```cpp
constexpr int8_t kTftCsPin = 10;
constexpr int8_t kTftDcPin = 9;
constexpr int8_t kTftResetPin = -1;
```

Change these values to match your hardware.

- **CS:** Chip Select, a configurable digital output
- **DC:** Data/Command, a configurable digital output
- **RST:** Display reset; use `-1` when connected to the board's hardware RESET signal
- **MOSI:** Connect to the board's hardware SPI MOSI pin
- **SCK:** Connect to the board's hardware SPI clock pin
- **MISO:** Not normally needed for this display-only application

Do not assume hardware SPI pins have the same numbering on different AVR boards.

### 4. Voltage compatibility

The Feather 32u4 operates at 3.3 V logic, while many Arduino Uno, Nano, and Mega boards operate at 5 V logic.

The ILI9341 controller uses 3.3 V logic. Some TFT breakout boards include voltage regulation or level shifting, while others do not.

Check the specifications of your particular display module before connecting it to a 5 V AVR board. A voltage regulator alone does not necessarily make the signal inputs 5 V tolerant.

### 5. Required libraries

Install the following libraries through the Arduino IDE Library Manager:

- Adafruit GFX Library
- Adafruit ILI9341

The Arduino SPI library is also required and is normally supplied with the board platform.

Select your board and processor in the Arduino IDE before compiling.

### 6. Memory considerations

AVR microcontrollers have limited flash and SRAM compared with ESP32 or RP2040 boards.

PackOS Lamda minimizes memory usage by:

- Using `F()` and `PROGMEM` for fixed text and lookup tables
- Avoiding full-screen framebuffers
- Redrawing only changed display segments
- Avoiding dynamic `String` objects
- Removing landscape orientation and unused touchscreen features

The optional `FreeMonoBold9pt7b` font can be disabled to save program flash:

```cpp
#define PACKOS_USE_TECH_FONT 0
```

Check the flash and SRAM usage reported by the Arduino compiler.

A successful compilation does not guarantee sufficient free SRAM during execution.

### 7. Display orientation

PackOS Lamda supports portrait mode only.

The display rotation is configured using:

```cpp
constexpr uint8_t kPortraitRotation = 0;
```

Some display modules may require rotation `2` for the opposite portrait orientation.

Landscape modes are not supported because the interface coordinates are designed for a 240 × 320 display.

### 8. Performance

The default display update interval is 180 milliseconds.

```cpp
constexpr uint16_t kRefreshIntervalMs = 180;
```

On slower AVR processors, increasing this interval to 250 or 300 milliseconds may improve responsiveness and reduce SPI traffic.

The animation uses floating-point calculations, which may be relatively expensive on 8-bit AVR processors.

### 9. Troubleshooting

**Blank screen:** Check CS, DC, RESET, MOSI, SCK, power and backlight connections.

**Display works but is upside down:** Try portrait rotation `2`.

**Compiler reports insufficient flash:** Disable the optional technical font or simplify the boot sequence.

**Unstable operation or random resets:** Check SRAM usage, power stability, wiring and display voltage compatibility.

**Slow animations:** Increase the refresh interval.

**Touchscreen does not work:** This is intentional. PackOS Lamda does not implement touchscreen functionality.

### 10. Contributing

Contributions and reports from users running PackOS Lamda on other AVR boards are welcome.

When reporting compatibility, please include:

- Microcontroller and board model
- Clock speed and operating voltage
- Display module model
- Pin assignments
- Arduino board-core and library versions
- Compiled flash and SRAM usage
- Any modifications required

This information will help build a verified compatibility list over time.

**Disclaimer:** PackOS Lamda simulates suit telemetry for costume and prop applications. It is not intended for actual environmental monitoring or life-support equipment.
