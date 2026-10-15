/*
 * PackOS Lambda
 * -------------
 * Author:   Philippe Thunderbolt
 * Project:  PackOS Pack Display for Belters for AVR Microcontrollers
 * Platform: AVR microcontrollers
 
 * PackOS Lamda is a lightweight, portrait-only suit-status display for Arduino-compatible AVR microcontrollers
 * optimized for the Adafruit Feather 32u4 and ILI9341 SPI TFT displays.
 * Full packOS is at https://github.com/PhilippeThunderbolt/PackOS
 *
 * Hardware I tested on:
 *   Adafruit Feather 32u4 Basic (ATmega32u4, 3.3 V logic)
 *   Any AVR microcontroller may work
 *   HiLetgo ILI9341 2.8" SPI TFT (240 x 320 pixels)
 *
 * Connections:
 *   TFT CS    -> Feather D10
 *   TFT DC    -> Feather D9
 *   TFT RST   -> Feather RESET (not a GPIO)
 *   TFT MOSI  -> Feather hardware MOSI
 *   TFT SCK   -> Feather hardware SCK
 *   TFT GND   -> GND
 *   TFT VCC / LED -> per your particular module's specifications
 *
 * Libraries: Adafruit GFX, Adafruit ILI9341 (and their dependencies).
 * Touch and SD card are not used.
 *
 * Behavior:
 *   - Seven-step animated Belter-style startup sequence.
 *   - Five simulated telemetry bars; oxygen drains over a timed cycle.
 *   - Incremental bar updates reduce unnecessary SPI transfers.
 *   - No framebuffer, dynamic String objects, or landscape mode.
 *
 * License: retain the license and attribution from the upstream PackOS
 * project when publishing a derivative. No license is asserted here.
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <math.h>
#include <string.h>

// A compact, technical-looking fixed-width font for the header/footer.
// The optional font consumes additional flash; disable if space is tight.
#define PACKOS_USE_TECH_FONT 1
#if PACKOS_USE_TECH_FONT
#include <Fonts/FreeMonoBold9pt7b.h>
#endif

namespace Config {
  // Physical display and connections.
  constexpr int8_t kTftCsPin = 10;
  constexpr int8_t kTftDcPin = 9;
  constexpr int8_t kTftResetPin = -1;  // Connected to Feather RESET.
  constexpr uint8_t kPortraitRotation = 0;
  constexpr int16_t kScreenWidth = 240;
  constexpr int16_t kScreenHeight = 320;

  // Display palette (RGB565).
  constexpr uint16_t kBackground = ILI9341_BLACK;
  constexpr uint16_t kPanel = 0x1082;
  constexpr uint16_t kUnlitSegment = 0x18C3;
  constexpr uint16_t kFrame = ILI9341_CYAN;
  constexpr uint16_t kText = ILI9341_WHITE;
  constexpr uint16_t kHealthy = ILI9341_GREEN;
  constexpr uint16_t kCaution = ILI9341_YELLOW;
  constexpr uint16_t kCritical = ILI9341_RED;

  // Main screen geometry, in pixels.
  constexpr int16_t kHeaderX = 6;
  constexpr int16_t kHeaderY = 6;
  constexpr int16_t kHeaderWidth = 228;
  constexpr int16_t kHeaderHeight = 52;
  constexpr int16_t kHeaderRadius = 6;
  constexpr int16_t kHeaderAccentLeftX = 16;
  constexpr int16_t kHeaderAccentRightX = 223;
  constexpr int16_t kHeaderAccentY = 14;
  constexpr int16_t kHeaderAccentHeight = 36;
  constexpr int16_t kTitleCenterX = 120;
  constexpr int16_t kTitleY = 24;
  constexpr int16_t kLabelY = 73;
  constexpr int16_t kBarStartX = 10;
  constexpr int16_t kBarPitch = 47;
  constexpr int16_t kBarWidth = 36;
  constexpr int16_t kBarTop = 92;
  constexpr int16_t kBarBottom = 278;
  constexpr int16_t kBarFramePadding = 2;
  constexpr int16_t kBarFrameRadius = 3;
  constexpr int16_t kSegmentHeight = 12;
  constexpr int16_t kSegmentGap = 4;
  constexpr int16_t kSegmentRadius = 2;
  constexpr int16_t kFooterX = 6;
  constexpr int16_t kFooterY = 285;
  constexpr int16_t kFooterWidth = 228;
  constexpr int16_t kFooterHeight = 29;
  constexpr int16_t kFooterRadius = 5;
  constexpr int16_t kFooterCenterX = 120;
  constexpr int16_t kFooterTextY = 293;

  // Text sizes for the built-in GFX 5x7 font.
  constexpr uint8_t kNormalTextSize = 1;
  constexpr uint8_t kLargeTextSize = 2;
  constexpr uint8_t kBootTitleTextSize = 3;
  constexpr int16_t kDefaultFontAdvance = 6;

  // Telemetry simulation.
  constexpr uint8_t kVitalCount = 5;
  constexpr uint8_t kFullScalePercent = 100;
  constexpr uint8_t kUninitializedLitCount = 255;
  constexpr uint16_t kOxygenCycleMinutes = 30;
  constexpr uint32_t kMillisecondsPerMinute = 60000UL;
  constexpr uint16_t kRefreshIntervalMs = 180;
  constexpr float kOxygenCurveExponent = 1.85f;
  constexpr float kAnimationTimeScale = 0.00028f;
  constexpr uint8_t kOxygenYellowThreshold = 15;
  constexpr uint8_t kOxygenRedThreshold = 5;

  // Boot screen geometry and pacing.
  constexpr int16_t kBootFrameX = 7;
  constexpr int16_t kBootFrameY = 8;
  constexpr int16_t kBootFrameWidth = 226;
  constexpr int16_t kBootFrameHeight = 304;
  constexpr int16_t kBootFrameRadius = 5;
  constexpr int16_t kBootTitleX = 38;
  constexpr int16_t kBootTitleY = 28;
  constexpr int16_t kBootSubtitleX = 31;
  constexpr int16_t kBootSubtitleY = 64;
  constexpr int16_t kBootDividerX = 16;
  constexpr int16_t kBootDividerY = 81;
  constexpr int16_t kBootDividerWidth = 208;
  constexpr int16_t kBootSectionX = 18;
  constexpr int16_t kBootSectionY = 96;
  constexpr int16_t kBootClearX = 16;
  constexpr int16_t kBootClearY = 133;
  constexpr int16_t kBootClearWidth = 208;
  constexpr int16_t kBootClearHeight = 85;
  constexpr int16_t kBootMessageX = 18;
  constexpr int16_t kBootMessageY = 140;
  constexpr int16_t kBootStatusY = 161;
  constexpr int16_t kBootProgressX = 17;
  constexpr int16_t kBootProgressY = 237;
  constexpr int16_t kBootProgressWidth = 206;
  constexpr int16_t kBootProgressHeight = 17;
  constexpr int16_t kBootProgressInset = 3;
  constexpr int16_t kBootProgressFillWidth = 200;
  constexpr int16_t kBootProgressFillHeight = 11;
  constexpr int16_t kBootLoadX = 18;
  constexpr int16_t kBootLoadY = 268;
  constexpr int16_t kBootCompleteX = 18;
  constexpr int16_t kBootCompleteY = 291;
  constexpr uint8_t kBootStepCount = 7;
  constexpr uint16_t kBootStepDelayMs = 320;
  constexpr uint16_t kBootCompleteDelayMs = 900;
}

// Adafruit ILI9341 uses the board's hardware SPI by default.
Adafruit_ILI9341 tft(Config::kTftCsPin,
                     Config::kTftDcPin,
                     Config::kTftResetPin);

// Flash-resident labels: ordinary arrays of string pointers waste AVR SRAM.
const char kLabelOxygen[] PROGMEM = "OXYGEN";
const char kLabelBreath[] PROGMEM = "BREATH";
const char kLabelCo2[] PROGMEM = "CO2";
const char kLabelHeart[] PROGMEM = "HEART";
const char kLabelPressure[] PROGMEM = "PRESS";
const char* const kVitalLabels[] PROGMEM = {
  kLabelOxygen, kLabelBreath, kLabelCo2, kLabelHeart, kLabelPressure
};

// Thresholds are indexed in the same order as the labels above.
// Read with pgm_read_byte() because they are stored in program memory.
const uint8_t kWarningThresholds[] PROGMEM = {60, 58, 60, 52, 50};
const uint8_t kCriticalThresholds[] PROGMEM = {28, 30, 32, 26, 24};

// Last drawn values allow the renderer to skip unchanged segments.
uint8_t previousLit[Config::kVitalCount];
uint16_t previousColor[Config::kVitalCount];
uint32_t oxygenCycleStartMs = 0;
uint32_t lastFrameMs = 0;

// Draw centered text using the small built-in font, without dynamic allocation.
void drawCenteredText(const char* text, int16_t centerX, int16_t y,
                      uint8_t size, uint16_t foreground,
                      uint16_t background) {
  tft.setFont(nullptr);
  tft.setTextSize(size);
  tft.setTextColor(foreground, background);
  tft.setCursor(centerX -
                    static_cast<int16_t>(strlen(text)) *
                    Config::kDefaultFontAdvance * size / 2,
                y);
  tft.print(text);
}

// The optional monospaced bold font gives a more technical look.
// GFX custom fonts use a baseline cursor, not a top-left cursor.
void drawTechnicalText(const __FlashStringHelper* text,
                       int16_t centerX, int16_t baseline,
                       uint16_t color) {
#if PACKOS_USE_TECH_FONT
  tft.setFont(&FreeMonoBold9pt7b);
  tft.setTextSize(1);
  tft.setTextColor(color);
  // FlashStringHelper prints without creating an SRAM copy.
  // Centering uses a fixed x origin for our known short headings.
  tft.setCursor(centerX, baseline);
  tft.print(text);
  tft.setFont(nullptr);
#else
  tft.setFont(nullptr);
  tft.setTextSize(Config::kLargeTextSize);
  tft.setTextColor(color);
  tft.setCursor(centerX, baseline - 14);
  tft.print(text);
#endif
}

// Calculate the left edge of one telemetry column.
int16_t barX(uint8_t index) {
  return Config::kBarStartX + Config::kBarPitch * index;
}

// Draw static geometry once. Dynamic bars are drawn separately.
void drawMainScreen() {
  tft.fillScreen(Config::kBackground);

  tft.fillRoundRect(Config::kHeaderX, Config::kHeaderY,
                    Config::kHeaderWidth, Config::kHeaderHeight,
                    Config::kHeaderRadius, Config::kPanel);
  tft.drawRoundRect(Config::kHeaderX, Config::kHeaderY,
                    Config::kHeaderWidth, Config::kHeaderHeight,
                    Config::kHeaderRadius, Config::kFrame);
  tft.drawFastVLine(Config::kHeaderAccentLeftX, Config::kHeaderAccentY,
                    Config::kHeaderAccentHeight, Config::kFrame);
  tft.drawFastVLine(Config::kHeaderAccentRightX, Config::kHeaderAccentY,
                    Config::kHeaderAccentHeight, Config::kFrame);

  // Fixed origin keeps the custom-font heading centered within the panel.
  drawTechnicalText(F("SUIT STATUS"), 54, 38, Config::kFrame);

  for (uint8_t i = 0; i < Config::kVitalCount; ++i) {
    char label[8];
    PGM_P flashLabel = reinterpret_cast<PGM_P>(
        pgm_read_ptr(&kVitalLabels[i]));
    strcpy_P(label, flashLabel);

    drawCenteredText(label, barX(i) + Config::kBarWidth / 2,
                     Config::kLabelY, Config::kNormalTextSize,
                     Config::kText, Config::kBackground);

    tft.drawRoundRect(barX(i) - Config::kBarFramePadding,
                      Config::kBarTop - Config::kBarFramePadding,
                      Config::kBarWidth + 2 * Config::kBarFramePadding,
                      Config::kBarBottom - Config::kBarTop +
                          2 * Config::kBarFramePadding,
                      Config::kBarFrameRadius, Config::kText);
  }

  tft.fillRoundRect(Config::kFooterX, Config::kFooterY,
                    Config::kFooterWidth, Config::kFooterHeight,
                    Config::kFooterRadius, Config::kPanel);
  tft.drawRoundRect(Config::kFooterX, Config::kFooterY,
                    Config::kFooterWidth, Config::kFooterHeight,
                    Config::kFooterRadius, Config::kFrame);

  
  drawTechnicalText(F("PackOS Lambda"), 47, 306, Config::kText);
}

// Return simulated oxygen remaining (0-100%) in the current cycle.
// Unsigned subtraction remains valid across millis() rollover.
uint8_t oxygenPercent(uint32_t nowMs) {
  const uint32_t durationMs =
      static_cast<uint32_t>(Config::kOxygenCycleMinutes) *
      Config::kMillisecondsPerMinute;
  const uint32_t elapsedMs = nowMs - oxygenCycleStartMs;

  if (elapsedMs >= durationMs) {
    return 0;
  }

  const float progress =
      static_cast<float>(elapsedMs) / static_cast<float>(durationMs);
  const float remaining =
      1.0f - powf(progress, Config::kOxygenCurveExponent);

  return static_cast<uint8_t>(Config::kFullScalePercent * remaining);
}

// Generate gently oscillating simulated values for the other four vitals.
// Coefficients are grouped here, rather than scattered through rendering.
struct WaveProfile {
  float base;
  float primaryAmplitude;
  float primaryFrequency;
  float primaryPhase;
  float secondaryAmplitude;
  float secondaryFrequency;
  float secondaryPhase;
};

const WaveProfile kWaveProfiles[] PROGMEM = {
  {0.58f, 0.12f, 0.22f, 0.9f, 0.03f, 0.05f, 2.3f}, // Breath
  {0.67f, 0.09f, 0.18f, 2.0f, 0.04f, 0.06f, 0.5f}, // CO2
  {0.54f, 0.14f, 0.31f, 1.6f, 0.03f, 0.09f, 2.8f}, // Heart
  {0.50f, 0.11f, 0.20f, 2.7f, 0.04f, 0.05f, 1.1f}  // Pressure
};

uint8_t vitalPercent(uint8_t index, float time, uint8_t oxygen) {
  if (index == 0) {
    return oxygen;
  }

  WaveProfile wave;
  memcpy_P(&wave, &kWaveProfiles[index - 1], sizeof(wave));
  const float value =
      wave.base +
      wave.primaryAmplitude *
          sinf(time * wave.primaryFrequency + wave.primaryPhase) +
      wave.secondaryAmplitude *
          sinf(time * wave.secondaryFrequency + wave.secondaryPhase);

  return static_cast<uint8_t>(
      constrain(static_cast<int>(value * Config::kFullScalePercent),
                0, Config::kFullScalePercent));
}

// Choose bar color based on the configured thresholds.
uint16_t vitalColor(uint8_t index, uint8_t percent) {
  const uint8_t critical = pgm_read_byte(&kCriticalThresholds[index]);
  const uint8_t warning = pgm_read_byte(&kWarningThresholds[index]);

  if (index == 0 && percent <= Config::kOxygenYellowThreshold) {
    return percent <= Config::kOxygenRedThreshold ?
        Config::kCritical : Config::kCaution;
  }
  if (percent < critical) {
    return Config::kCritical;
  }
  if (percent < warning) {
    return Config::kCaution;
  }
  return Config::kHealthy;
}

// Update only segments whose lit state or active color has changed.
void renderBar(uint8_t index, uint8_t percent) {
  constexpr uint8_t kSegmentCount =
      (Config::kBarBottom - Config::kBarTop) /
      (Config::kSegmentHeight + Config::kSegmentGap);

  const uint16_t color = vitalColor(index, percent);
  uint8_t lit = static_cast<uint16_t>(percent) * kSegmentCount /
                Config::kFullScalePercent;
  if (percent == Config::kFullScalePercent) {
    lit = kSegmentCount;
  }

  if (lit == previousLit[index] && color == previousColor[index]) {
    return;
  }

  for (uint8_t segment = 0; segment < kSegmentCount; ++segment) {
    const bool wasLit =
        previousLit[index] != Config::kUninitializedLitCount &&
        segment < previousLit[index];
    const bool isLit = segment < lit;

    if (previousLit[index] != Config::kUninitializedLitCount &&
        wasLit == isLit &&
        (!isLit || color == previousColor[index])) {
      continue;
    }

    const int16_t y =
        Config::kBarBottom - Config::kSegmentHeight -
        segment * (Config::kSegmentHeight + Config::kSegmentGap);
    tft.fillRoundRect(barX(index), y, Config::kBarWidth,
                      Config::kSegmentHeight, Config::kSegmentRadius,
                      isLit ? color : Config::kUnlitSegment);
  }

  previousLit[index] = lit;
  previousColor[index] = color;
}

// The switch keeps every diagnostic string in flash through F().
void printBootMessage(uint8_t step) {
  switch (step) {
    case 0: tft.print(F("> pak os wakey")); break;
    case 1: tft.print(F("> fo check air")); break;
    case 2: tft.print(F("> co2 scrubba")); break;
    case 3: tft.print(F("> helmet hud")); break;
    case 4: tft.print(F("> pressure seal")); break;
    case 5: tft.print(F("> suit telem")); break;
    default: tft.print(F("> no leak, kopeng")); break;
  }
}

// Brief startup animation; blocking delays are intentional at power-on.
void showBootSequence() {
  tft.fillScreen(Config::kBackground);
  tft.setTextWrap(false);
  tft.drawRoundRect(Config::kBootFrameX, Config::kBootFrameY,
                    Config::kBootFrameWidth, Config::kBootFrameHeight,
                    Config::kBootFrameRadius, Config::kFrame);

  tft.setTextColor(Config::kFrame);
  tft.setTextSize(Config::kBootTitleTextSize);
  tft.setCursor(Config::kBootTitleX, Config::kBootTitleY);
  tft.print(F("PackOS"));

  tft.setTextSize(Config::kNormalTextSize);
  tft.setCursor(Config::kBootSubtitleX, Config::kBootSubtitleY);
  tft.print(F("CANTERBURY / SUIT SYSTEM"));
  tft.drawFastHLine(Config::kBootDividerX, Config::kBootDividerY,
                    Config::kBootDividerWidth, Config::kFrame);

  tft.setTextColor(Config::kHealthy);
  tft.setTextSize(Config::kLargeTextSize);
  tft.setCursor(Config::kBootSectionX, Config::kBootSectionY);
  tft.print(F("BELTER SUITCHECK"));

  for (uint8_t step = 0; step < Config::kBootStepCount; ++step) {
    tft.fillRect(Config::kBootClearX, Config::kBootClearY,
                 Config::kBootClearWidth, Config::kBootClearHeight,
                 Config::kBackground);
    tft.setTextColor(Config::kText);
    tft.setTextSize(Config::kNormalTextSize);
    tft.setCursor(Config::kBootMessageX, Config::kBootMessageY);
    printBootMessage(step);
    tft.setCursor(Config::kBootMessageX, Config::kBootStatusY);
    tft.print(F("SYSTEMS NOMINAL"));

    tft.drawRect(Config::kBootProgressX, Config::kBootProgressY,
                 Config::kBootProgressWidth, Config::kBootProgressHeight,
                 Config::kFrame);
    const int16_t fillWidth =
        static_cast<uint32_t>(Config::kBootProgressFillWidth) *
        (step + 1) / Config::kBootStepCount;
    tft.fillRect(Config::kBootProgressX + Config::kBootProgressInset,
                 Config::kBootProgressY + Config::kBootProgressInset,
                 fillWidth, Config::kBootProgressFillHeight,
                 Config::kHealthy);

    // Calculate the current loading percentage.
const uint8_t loadPercent =
    static_cast<uint32_t>(Config::kFullScalePercent) *
    (step + 1) / Config::kBootStepCount;

// Clear the previous percentage before drawing the new one.
// This prevents overlapping digits from appearing as solid blocks.
tft.fillRect(
    Config::kBootLoadX,
    Config::kBootLoadY,
    120, 12,
    Config::kBackground
);

// Draw the updated loading percentage.
tft.setFont(nullptr);  // Use the built-in GFX font.
tft.setTextSize(Config::kNormalTextSize);
tft.setTextColor(Config::kFrame, Config::kBackground);
tft.setCursor(Config::kBootLoadX, Config::kBootLoadY);

tft.print(F("LOAD "));
tft.print(loadPercent);
tft.print('%');

delay(Config::kBootStepDelayMs);
  }

  tft.setTextColor(Config::kHealthy);
  tft.setCursor(Config::kBootCompleteX, Config::kBootCompleteY);
  tft.print(F("kopeng. suit stay running."));
  delay(Config::kBootCompleteDelayMs);
}

// Mark all segments dirty so they are drawn on the first frame.
void invalidateBars() {
  for (uint8_t i = 0; i < Config::kVitalCount; ++i) {
    previousLit[i] = Config::kUninitializedLitCount;
    previousColor[i] = 0;
  }
}

void setup() {
  tft.begin();
  tft.setRotation(Config::kPortraitRotation);
  showBootSequence();
  drawMainScreen();
  invalidateBars();

  oxygenCycleStartMs = millis();
  lastFrameMs = oxygenCycleStartMs - Config::kRefreshIntervalMs;
}

void loop() {
  const uint32_t nowMs = millis();

  // Non-blocking frame limiter; unsigned arithmetic handles millis rollover.
  if (nowMs - lastFrameMs < Config::kRefreshIntervalMs) {
    return;
  }
  lastFrameMs = nowMs;

  uint8_t oxygen = oxygenPercent(nowMs);
  if (oxygen == 0) {
    // Start a new simulated oxygen cycle once the old one is exhausted.
    oxygenCycleStartMs = nowMs;
    oxygen = Config::kFullScalePercent;
    invalidateBars();
  }

  const float animationTime =
      (nowMs - oxygenCycleStartMs) * Config::kAnimationTimeScale;

  for (uint8_t i = 0; i < Config::kVitalCount; ++i) {
    renderBar(i, vitalPercent(i, animationTime, oxygen));
  }
}

