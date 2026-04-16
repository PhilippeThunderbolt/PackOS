/// =====================================================
// OPA PACKOS – PACK VITALS DISPLAY
// -----------------------------------------------------
// Author: Philippe Thunderbolt
// Project: PackOS Pack Display for Belters
// Platform: Arduino / ESP32 + TFT display
// Library: TFT_eSPI
// Date:  4/13/26
//
// Description:
// This sketch implements a sci-fi-themed "Belter" suit
// status display inspired by The Expanse. It includes:
//
// - Boot sequence with animated loading and console output
// - Real-time vitals bar display (Oxygen, Breath, CO2, etc.)
// - Low oxygen warning and critical alert modes
// - Animated UI elements and optional glitch effects
// - Support for portrait and landscape orientation
//
// -----------------------------------------------------
// DESIGN NOTES:
// This UI is intentionally tuned for a 240x320 / 320x240
// class TFT display. Layout values are not fully scalable.
//
// Portrait mode is the primary design target.
// Landscape mode is simplified for readability.

// Supported platforms:
// - ESP32-based boards (ESP32, ESP32-S2, ESP32-S3) — fully supported
//   e.g. Adafruit Feather ESP32 V2, ESP32-S2, ESP32-S3
//
// - RP2040 boards (e.g. Adafruit Feather RP2040) — supported with TFT_eSPI
//   (requires Earle Philhower core)
//
// Notes:
// - Requires TFT_eSPI library configured for the target display
// - Not supported on AVR-based boards (e.g. Feather 32u4, ATmega328P)
// - SAMD21 (Feather M0) may run a reduced/slow version with alternate libraries
// -----------------------------------------------------
// VERSION HISTORY:
// v4.13.26  Initial public release
//
// -----------------------------------------------------
// CREDITS:
// Based on original backpack code written by Mark Perino
// and modified by Dan Shope in April 2021.
//
// -----------------------------------------------------
// NOTES:
// This code is designed for visual effect and readability,
// not strict engineering accuracy.

//Pin definitions for the display must be made in the TFT_eSPI 
//library setup file
//
// "Beltalowda forever."
// =====================================================

constexpr const char* BOOT_VERSION = "v4.13.26"; // version of this program

#include <TFT_eSPI.h>
#include <math.h>
#include <string.h>

TFT_eSPI tft = TFT_eSPI();

// =====================================================
// QUICK SETUP - SAFE TO EDIT
// =====================================================

// Boot screen text
constexpr const char* BOOT_TITLE = "PackOS"; //OS Name
constexpr const char* BOOT_SYSTEM = "CANTERBURY ";  //This is the name of your ship

// Main title at the top of the display
constexpr const char* PANEL_TITLE = "SUIT STATUS";

// Screen direction:
// true  = portrait
// false = landscape
constexpr bool USE_PORTRAIT_MODE = true;

// How active the display feels
// 1 = very calm
// 5 = balanced
// 10 = very active / busy
constexpr uint8_t ACTIVITY_LEVEL = 5;

// Turn all visual effects on or off
// Includes faded scanlines AND glitch effects
// false = clean display
// true  = sci-fi effects
constexpr bool SHOW_GLITCH = false;

// Show a fake OS loading sequence on the boot screen
constexpr bool SHOW_BOOT_LOADING = true;

// Test mode: start oxygen near the pre-alert zone for quick testing
constexpr bool ENABLE_LOW_OXYGEN_TEST_MODE = false;

// =====================================================
// Timers that you can play with
// =====================================================

// Boot text typing speed in milliseconds per step
constexpr uint16_t BOOT_TYPE_SPEED_MS = 10;

// How long the progress bar animation takes per boot step
constexpr uint16_t BOOT_PROGRESS_STEP_MS = 140;

// Pause after boot reaches 100%, in milliseconds
constexpr uint16_t BOOT_COMPLETE_PAUSE_MS = 4000;

// How long oxygen takes to drain from full to empty
constexpr uint16_t OXYGEN_DRAIN_MINUTES = 30;

// Low-oxygen pre-alert threshold as a percent
constexpr uint8_t LOW_OXYGEN_PREALERT_PERCENT = 15;

// Critical oxygen threshold as a percent
constexpr uint8_t LOW_OXYGEN_CRITICAL_PERCENT = 5;

// Starting oxygen percent when test mode is enabled
constexpr uint8_t LOW_OXYGEN_TEST_START_PERCENT = 12;


// =====================================================
// UI TUNING CONSTANTS
// =====================================================

// Alert / reboot text
constexpr const char* LOW_OXYGEN_TEXT = "LOW OXYGEN";
constexpr const char* CRITICAL_ALERT_LINE1 = "CRITICAL";
constexpr const char* CRITICAL_ALERT_LINE2 = "OXYGEN ALERT";
constexpr const char* REBOOT_LINE1 = "GOODBYE";
constexpr const char* REBOOT_LINE2 = "BERATNA";

// Boot console text
constexpr const char* BOOT_NODE_LABEL = "SHIP:";
constexpr const char* BOOT_MODE_LABEL = "MODE:";
constexpr const char* BOOT_MODE_VALUE = "BELTER SUITCHECK";
constexpr const char* BOOT_FINAL_LINE1 = "all system nomina,";
constexpr const char* BOOT_FINAL_LINE2 = "kopeng. suit stay running.";
constexpr const char* BOOT_LOAD_LABEL = "LOAD";
constexpr const char* BOOT_LOAD_COMPLETE_TEXT = "LOAD 100%";
constexpr const char* BOOT_PROMPT_PREFIX = "> ";
constexpr const char* BOOT_SECOND_LINE_PREFIX = "  ";

// Boot module lines - editable at top of file
static const char* BOOT_LINE1_TABLE[] = {
  "pak os wakey",
  "fo check air",
  "o2 line",
  "co2 scrubba",
  "recycla spin",
  "helmet hud",
  "mag boots",
  "comms fo",
  "opa channel",
  "pressure seal?",
  "pressure seal",
  "thermal line",
  "heart beat",
  "injector primed",
  "suit telem",
  "gyro stay",
  "power bus",
  "vac sensa",
  "helmet seal",
  "suit core",
  "clamp and latch",
  "maneuva pak",
  "fo beltalowda",
  "no leak"
};

static const char* BOOT_LINE2_TABLE[] = {
  "now, kopeng...",
  "you set or no?",
  "fo wake up...",
  "run now...",
  "up, sasa...",
  "fo sync...",
  "lock, gut...",
  "handshake...",
  "open now...",
  "you sure?",
  "fo hold...",
  "fo align...",
  "fo track...",
  "beratna...",
  "streamin...",
  "no spin...",
  "route clear...",
  "fo calibrate...",
  "tight, ke?",
  "shake hand...",
  "nomina...",
  "stand by...",
  "all set...",
  "no problem..."
};

// Alert timing
constexpr uint16_t PREALERT_PULSE_MS_NORMAL = 500;
constexpr uint16_t PREALERT_PULSE_MS_FAST = 350;
constexpr uint16_t CRITICAL_ALERT_PULSE_MS = 140;

// Reboot timing
constexpr uint16_t REBOOT_MESSAGE_DELAY_MS = 3000;

// Boot screen layout
constexpr int BOOT_MAIN_BOX_HEIGHT = 150;
constexpr int BOOT_TERMINAL_BOX_HEIGHT = 68;
constexpr int BOOT_TERMINAL_BOX_HEIGHT_ANIM = 56;

// Progress bar / load readout
constexpr int LOAD_BAR_HEIGHT = 14;

// Alert box tuning
constexpr int LOW_OXYGEN_BOX_BOTTOM_TRIM = 4;
constexpr int BOOT_LINE_COUNT = sizeof(BOOT_LINE1_TABLE) / sizeof(BOOT_LINE1_TABLE[0]);

// Headings above the bars
constexpr int NUM_BARS = 5;
constexpr const char* HEADINGS[NUM_BARS] = {
  "OXYGEN",
  "BREATH",
  "CO2",
  "HEART",
  "PRESS"
};

// How often the screen updates, in milliseconds
constexpr uint16_t FRAME_MS = 50;

// How long a random red event lasts
constexpr uint8_t RANDOM_RED_EVENT_SECONDS = 4;

// How often glitch effects are checked
constexpr uint16_t GLITCH_CHECK_MS = 220;

// Base chance of a glitch burst when checked
constexpr uint8_t BASE_GLITCH_CHANCE_PERCENT = 8;

// How often pixel flicker is checked
constexpr uint16_t PIXEL_FLICKER_CHECK_MS = 90;

// Base number of pixels in one flicker burst
constexpr uint8_t PIXEL_FLICKER_BASE_COUNT = 6;

// Max extra flicker pixels added at high activity
constexpr uint8_t PIXEL_FLICKER_ACTIVITY_BOOST = 10;

// Thresholds for bar colors, as percentages
constexpr int WARN_THRESHOLD[5] = { 60, 58, 60, 52, 50 };
constexpr int BAD_THRESHOLD[5] = { 28, 30, 32, 26, 24 };

// Segment look for the bar blocks
constexpr int SEGMENT_HEIGHT = 12;
constexpr int SEGMENT_GAP = 4;

// Number of flashes when a bar first turns red
constexpr uint8_t RED_FLASH_TOGGLES = 10;

// =====================================================
// DO NOT EDIT BELOW THIS LINE UNLESS YOU KNOW WHAT YOU'RE DOING
// =====================================================

#define TFT_ROTATION (USE_PORTRAIT_MODE ? 0 : 1)

static const uint16_t BG_COLOR = TFT_BLACK;
static const uint16_t PANEL_COLOR = 0x1082;
static const uint16_t FRAME_COLOR = TFT_WHITE;
static const uint16_t ACCENT_COLOR = TFT_CYAN;
static const uint16_t TITLE_COLOR = TFT_CYAN;
static const uint16_t SUB_COLOR = TFT_YELLOW;
static const uint16_t TEXT_COLOR = TFT_WHITE;
static const uint16_t DIM_TEXT = 0x8410;
static const uint16_t PULSE_COLOR = 0x867D;

static const uint16_t GOOD_COLOR = TFT_GREEN;
static const uint16_t WARN_COLOR = TFT_YELLOW;
static const uint16_t BAD_COLOR = TFT_RED;
static const uint16_t OFF_SEG_COLOR = 0x18C3;

constexpr unsigned long O2_CYCLE_MS = (unsigned long)OXYGEN_DRAIN_MINUTES * 60UL * 1000UL;

// Activity-driven internal values
float BAR_ANIMATION_SPEED = 0.010f;
uint8_t RANDOM_RED_EVENT_CHANCE_PERCENT = 8;
uint8_t glitchChancePercent = BASE_GLITCH_CHANCE_PERCENT;
uint8_t pixelFlickerCount = PIXEL_FLICKER_BASE_COUNT;

int SCREEN_W, SCREEN_H;
int headerTop, headerH;
int titleY;
int headingY;
int barTop, barBottom, barH;
int barW, barGap;
int readoutY, readoutH;
int segHeight, segGap;
int barX[NUM_BARS];

uint8_t prevStates[NUM_BARS] = { 255, 255, 255, 255, 255 };

bool flashActive[NUM_BARS] = { false, false, false, false, false };
uint8_t flashCount[NUM_BARS] = { 0, 0, 0, 0, 0 };
bool flashVisible[NUM_BARS] = { false, false, false, false, false };

float animT = 0.0f;
unsigned long lastFrame = 0;
unsigned long cycleStartMs = 0;

bool lowOxygenPreAlertActive = false;

uint16_t cpuLoadEstimateTenths = 0;

unsigned long lastPixelFlickerMs = 0;

bool redEventActive = false;
int redEventBar = -1;
unsigned long redEventStartMs = 0;
unsigned long lastRedEventCheckMs = 0;

bool headerDimState = false;
unsigned long lastGlitchCheckMs = 0;

// =====================================================
// Function declarations
// =====================================================
void applyActivityLevel();
void computeLayout();
void initCycle();
void drawBootScreen();
void runBootLoadingAnimation();

void drawTwoLineTypewriterText(const char* line1, const char* line2, int x, int y, int maxWidth, uint16_t color, int font, int delayMs);
void animateLoadBarToTarget(int& currentFillW, int targetFillW, int loadBarX, int loadBarY, int loadBarH, uint16_t pulseColor, uint16_t durationMs);
void drawStaticUI();
void drawHeaderPanel();
void drawBarFrames();
void drawHeadingLabels();
void updateBars();
void drawSegmentedBar(int idx, int height, uint16_t color, bool fullFlash);
int vitalHeight(int idx, float t);
uint8_t vitalState(int idx, int percent);
uint16_t stateColor(uint8_t state);
void startRedFlash(int idx);
void updateFlashState(int idx);
void drawReadoutPanel();
void updateReadoutPanel();
int oxygenPercentFromTime();
int heightFromPercent(int percent);

void resetCycle();
void applyLowOxygenTestMode();
void rebootPack();
int pickTitleFont();
int pickLabelFont();
int pickReadoutFont();
void updateEffects();
void updatePixelFlicker();
void drawPixelFlickerBurst(int count);
void updateOccasionalRedEvent();
bool isRedEventBar(int idx);
void glitchEffect();
void redrawDynamicArea();

// =====================================================
// Setup / loop
// =====================================================
void setup() {
  Serial.begin(115200);
  randomSeed(micros());
  applyActivityLevel();

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(TFT_ROTATION);

  SCREEN_W = tft.width();
  SCREEN_H = tft.height();

  computeLayout();
  drawBootScreen();

  if (SHOW_BOOT_LOADING) {
    runBootLoadingAnimation();
  } else {
    delay(200);
  }

  resetCycle();
}

void loop() {
  unsigned long now = millis();
  int oxygenPct = oxygenPercentFromTime();

  if (oxygenPct <= 0) {
    rebootPack();
    return;
  }

  if (oxygenPct <= LOW_OXYGEN_CRITICAL_PERCENT) {
    handleCriticalOxygenAlert();
    return;
  }

  if (now - lastFrame >= FRAME_MS) {
    unsigned long workStart = micros();

    lastFrame = now;
    animT += BAR_ANIMATION_SPEED;

    updateOccasionalRedEvent();
    updateBars();
    updateReadoutPanel();
    updateEffects();

    if (oxygenPct <= LOW_OXYGEN_PREALERT_PERCENT) {
      handleLowOxygenPreAlert(oxygenPct);
    }

    unsigned long workTime = micros() - workStart;
    unsigned long loadTenths = (1000UL * workTime) / ((unsigned long)FRAME_MS * 1000UL);
    if (loadTenths > 1000UL) loadTenths = 1000UL;
    cpuLoadEstimateTenths = (uint16_t)loadTenths;
  }
}

// =====================================================
// Activity setup

// =====================================================
void applyActivityLevel() {
  int level = constrain(ACTIVITY_LEVEL, 1, 10);

  glitchChancePercent = constrain(2 + (level * 2), 2, 25);
  pixelFlickerCount = PIXEL_FLICKER_BASE_COUNT + ((level * PIXEL_FLICKER_ACTIVITY_BOOST) / 10);

  float baseSpeed = 0.010f;
  int baseChance = 8;

  float speedScale;
  float chanceScale;

  if (level == 5) {
    speedScale = 1.0f;
    chanceScale = 1.0f;
  } else if (level < 5) {
    speedScale = 0.55f + (level * 0.09f);
    chanceScale = 0.45f + (level * 0.11f);
  } else {
    speedScale = 1.0f + ((level - 5) * 0.12f);
    chanceScale = 1.0f + ((level - 5) * 0.15f);
  }

  BAR_ANIMATION_SPEED = baseSpeed * speedScale;
  RANDOM_RED_EVENT_CHANCE_PERCENT = (uint8_t)(baseChance * chanceScale);

  if (BAR_ANIMATION_SPEED < 0.004f) BAR_ANIMATION_SPEED = 0.004f;
  if (BAR_ANIMATION_SPEED > 0.020f) BAR_ANIMATION_SPEED = 0.020f;
  if (RANDOM_RED_EVENT_CHANCE_PERCENT > 20) RANDOM_RED_EVENT_CHANCE_PERCENT = 20;
}

// =====================================================
// Layout
// =====================================================
void computeLayout() {
  headerTop = max(4, SCREEN_H / 80);
  headerH = max(52, SCREEN_H / 6);

  readoutH = max(28, SCREEN_H / 11);
  readoutY = SCREEN_H - readoutH - 6;

  titleY = headerTop + (headerH - 26) / 2;
  headingY = headerTop + headerH + 8;

  barTop = headingY + 16;
  barBottom = readoutY - 8;
  barH = barBottom - barTop;

  barGap = max(6, SCREEN_W / 24);
  barW = (SCREEN_W - (barGap * (NUM_BARS + 1))) / NUM_BARS;

  int startX = barGap;
  for (int i = 0; i < NUM_BARS; i++) {
    barX[i] = startX + i * (barW + barGap);
  }

  segHeight = max(8, (SEGMENT_HEIGHT * SCREEN_H) / 320);
  segGap = max(2, (SEGMENT_GAP * SCREEN_H) / 320);
}

int pickTitleFont() {
  return 4;
}
int pickLabelFont() {
  return SCREEN_W >= 320 ? 2 : 1;
}
int pickReadoutFont() {
  return 2;
}

// =====================================================
// Reset / alert flow
// =====================================================
void initCycle() {
  cycleStartMs = millis();
  applyLowOxygenTestMode();
  lastFrame = millis();
  lastGlitchCheckMs = millis();

  redEventActive = false;
  redEventBar = -1;
  redEventStartMs = 0;
  lastRedEventCheckMs = millis();
  headerDimState = false;
  lowOxygenPreAlertActive = false;

  for (int i = 0; i < NUM_BARS; i++) {
    prevStates[i] = 255;
    flashActive[i] = false;
    flashCount[i] = 0;
    flashVisible[i] = false;
  }
}

void resetCycle() {
  initCycle();
  tft.fillScreen(BG_COLOR);
  drawStaticUI();
}

void applyLowOxygenTestMode() {
  if (!ENABLE_LOW_OXYGEN_TEST_MODE) return;

  int startPercent = constrain(LOW_OXYGEN_TEST_START_PERCENT, 1, 99);
  float remaining = (float)startPercent / 100.0f;
  float normalizedElapsed = powf(1.0f - remaining, 1.0f / 1.85f);
  unsigned long elapsedOffsetMs = (unsigned long)(normalizedElapsed * (float)O2_CYCLE_MS);

  cycleStartMs = millis() - elapsedOffsetMs;
}

void drawFullScreenAlert(uint16_t bg, uint16_t fg, const char* line1, const char* line2) {
  tft.fillScreen(bg);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(fg, bg);

  int centerX = SCREEN_W / 2;
  int centerY = SCREEN_H / 2;

  // BIG text (font 4 is largest built-in)
  tft.drawCentreString(line1, centerX, centerY - 28, 4);
  tft.drawCentreString(line2, centerX, centerY + 6, 4);
}

void handleLowOxygenPreAlert(int oxygenPct) {
  static unsigned long lastPulse = 0;
  static bool showAlert = false;

  unsigned long now = millis();
  uint16_t pulseMs = PREALERT_PULSE_MS_NORMAL;
  if (oxygenPct <= 10) pulseMs = PREALERT_PULSE_MS_FAST;

  if (now - lastPulse < pulseMs) return;
  lastPulse = now;
  showAlert = !showAlert;

  int boxX = 10;
  int boxY = 6;
  int boxW = SCREEN_W - 20;
  int boxH = max(30, headerTop + headerH - boxY + 2) - LOW_OXYGEN_BOX_BOTTOM_TRIM;

  if (showAlert) {
    tft.fillRect(boxX, boxY, boxW, boxH, TFT_YELLOW);
    tft.drawRect(boxX, boxY, boxW, boxH, TFT_BLACK);
    tft.drawRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, TFT_BLACK);

    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_BLACK, TFT_YELLOW);
    int centerY = boxY + (boxH / 2);

    // smaller + cleaner
    tft.drawCentreString(LOW_OXYGEN_TEXT, SCREEN_W / 2, centerY - 12, 4);
   
  } else {
    tft.fillRect(0, 0, SCREEN_W, headerTop + headerH + 6, BG_COLOR);
    drawHeaderPanel();
  }
}

void handleCriticalOxygenAlert() {
  static unsigned long lastPulse = 0;
  static bool invert = false;

  unsigned long now = millis();

  if (now - lastPulse >= CRITICAL_ALERT_PULSE_MS) {
    lastPulse = now;
    invert = !invert;

    uint16_t bg = invert ? TFT_RED : TFT_BLACK;
    uint16_t fg = invert ? TFT_BLACK : TFT_RED;

    drawFullScreenAlert(bg, fg, CRITICAL_ALERT_LINE1, CRITICAL_ALERT_LINE2);

    tft.drawRect(0, 0, SCREEN_W, SCREEN_H, TFT_RED);
    tft.drawRect(1, 1, SCREEN_W - 2, SCREEN_H - 2, TFT_RED);
  }
}

void rebootPack() {

  tft.fillScreen(BG_COLOR);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_RED, BG_COLOR);
  tft.setTextSize(2);
  tft.drawCentreString(REBOOT_LINE1, SCREEN_W / 2, (SCREEN_H / 2) - 20, 2);
  tft.drawCentreString(REBOOT_LINE2, SCREEN_W / 2, (SCREEN_H / 2) + 10, 2);
  tft.setTextSize(1);
  delay(REBOOT_MESSAGE_DELAY_MS);

  tft.fillScreen(BG_COLOR);
  drawBootScreen();

  if (SHOW_BOOT_LOADING) {
    runBootLoadingAnimation();
  } else {
    delay(200);
  }
}


bool canShowBootExtraText() {
  const int mainBoxY = 16;
  const int mainBoxBottom = mainBoxY + BOOT_MAIN_BOX_HEIGHT;
  const int termBoxTop = mainBoxBottom + 12;
  const int termBoxBottom = termBoxTop + BOOT_TERMINAL_BOX_HEIGHT_ANIM;
  const int percentY = SCREEN_H - 68;
  const int minGap = 8;

  return (percentY - termBoxBottom) >= minGap;
}

void drawBootScreen() {
  tft.fillScreen(BG_COLOR);
  tft.setTextDatum(TC_DATUM);

  // ==============================
  // TOP MAIN BOX
  // ==============================
  int mainBoxX = 10;
  int mainBoxY = 16;
  int mainBoxW = SCREEN_W - 20;
  int mainBoxH = BOOT_MAIN_BOX_HEIGHT;

  tft.drawRoundRect(mainBoxX, mainBoxY, mainBoxW, mainBoxH, 8, ACCENT_COLOR);
  tft.drawRoundRect(mainBoxX + 3, mainBoxY + 3, mainBoxW - 6, mainBoxH - 6, 8, BG_COLOR);

  tft.drawFastHLine(mainBoxX + 16, mainBoxY + 14, mainBoxW - 32, ACCENT_COLOR);

  tft.setTextColor(TITLE_COLOR, BG_COLOR);
  tft.drawCentreString(BOOT_TITLE, SCREEN_W / 2, mainBoxY + 38, 4);

  tft.setTextColor(SUB_COLOR, BG_COLOR);
  tft.drawCentreString(BOOT_VERSION, SCREEN_W / 2, mainBoxY + 64, 2);

  tft.drawFastHLine(mainBoxX + 16, mainBoxY + 86, mainBoxW - 32, ACCENT_COLOR);

  tft.setTextDatum(TL_DATUM);

  tft.setTextColor(DIM_TEXT, BG_COLOR);
  tft.drawString(BOOT_NODE_LABEL, mainBoxX + 18, mainBoxY + 98, 2);

  tft.setTextColor(TEXT_COLOR, BG_COLOR);
  tft.drawString(BOOT_SYSTEM, mainBoxX + 78, mainBoxY + 98, 2);

      tft.setTextColor(DIM_TEXT, BG_COLOR);
    tft.drawString(BOOT_MODE_LABEL, mainBoxX + 18, mainBoxY + 118, 2);

    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.drawString(BOOT_MODE_VALUE, mainBoxX + 78, mainBoxY + 118, 2);
  
  // ==============================
  // LOWER TERMINAL BOX
  // ==============================
  if (canShowBootExtraText()) {
    int termBoxX = 26;
    int termBoxY = mainBoxY + mainBoxH + 12;
    int termBoxW = SCREEN_W - 52;
    int termBoxH = BOOT_TERMINAL_BOX_HEIGHT;

    for (int i = 0; i < 3; i++) {
      tft.drawRect(termBoxX - i, termBoxY - i, termBoxW + (i * 2), termBoxH + (i * 2), ACCENT_COLOR);
    }

    tft.fillRect(termBoxX + 2, termBoxY + 2, termBoxW - 4, termBoxH - 4, BG_COLOR);
    tft.drawRect(termBoxX + 2, termBoxY + 2, termBoxW - 4, termBoxH - 4, DIM_TEXT);
  }
}

void runBootLoadingAnimation() {
  int mainBoxX = 10;
  int mainBoxY = 16;
  int mainBoxW = SCREEN_W - 20;
  int mainBoxH = BOOT_MAIN_BOX_HEIGHT;

  int termBoxX = 26;
  int termBoxY = mainBoxY + mainBoxH + 12;
  int termBoxW = SCREEN_W - 52;
  int termBoxH = BOOT_TERMINAL_BOX_HEIGHT_ANIM;

  int textX = termBoxX + 12;
  int textY = termBoxY + 10;
  int textW = termBoxW - 24;
  int textH = 42;

  int loadBarX = 16;
  int loadBarW = SCREEN_W - 32;
  int loadBarH = LOAD_BAR_HEIGHT;
  int loadBarY = SCREEN_H - 44;

  int percentY = SCREEN_H - 68;

  tft.setTextDatum(TL_DATUM);

  if (canShowBootExtraText()) {
    // Draw terminal box only when there is enough vertical room
    for (int i = 0; i < 3; i++) {
      tft.drawRect(termBoxX - i, termBoxY - i, termBoxW + (i * 2), termBoxH + (i * 2), ACCENT_COLOR);
    }

    tft.drawRect(termBoxX + 2, termBoxY + 2, termBoxW - 4, termBoxH - 4, DIM_TEXT);
  } else {
    // Landscape: completely remove terminal area so no text can appear there
    tft.fillRect(termBoxX - 3, termBoxY - 3, termBoxW + 6, termBoxH + 6, BG_COLOR);
  }

  tft.drawRoundRect(loadBarX, loadBarY, loadBarW, loadBarH, 4, ACCENT_COLOR);
  tft.fillRoundRect(loadBarX + 2, loadBarY + 2, loadBarW - 4, loadBarH - 4, 3, BG_COLOR);

  int currentFillW = 0;

  for (int step = 0; step < BOOT_LINE_COUNT; step++) {
    int targetFillW = ((loadBarW - 4) * step) / BOOT_LINE_COUNT;
    int pct = (100 * step + 1) / BOOT_LINE_COUNT;

    bool warnFlash = (random(100) < 14);
    bool pressureFlash = (step == 9 || step == 10);

    if (canShowBootExtraText()) {
      const char* line1 = BOOT_LINE1_TABLE[step];
      const char* line2 = BOOT_LINE2_TABLE[step];

      tft.fillRect(textX, textY, textW, textH, BG_COLOR);
      drawTwoLineTypewriterText(
        line1,
        line2,
        textX,
        textY,
        textW,
        pressureFlash ? BAD_COLOR : (warnFlash ? WARN_COLOR : TEXT_COLOR),
        2,
        BOOT_TYPE_SPEED_MS
      );
    }

    uint16_t pulseColor = ACCENT_COLOR;
    if (pressureFlash) pulseColor = BAD_COLOR;
    else if (warnFlash) pulseColor = WARN_COLOR;
    else if ((step % 2) == 0) pulseColor = PULSE_COLOR;

    animateLoadBarToTarget(
      currentFillW,
      targetFillW,
      loadBarX,
      loadBarY,
      loadBarH,
      pulseColor,
      BOOT_PROGRESS_STEP_MS
    );

    tft.fillRect(12, percentY, SCREEN_W - 24, 16, BG_COLOR);
    char pctText[24];
    snprintf(pctText, sizeof(pctText), "%s %3d%%", BOOT_LOAD_LABEL, pct);
    tft.setTextColor(TEXT_COLOR, BG_COLOR);
    tft.drawString(pctText, 16, percentY, 2);
  }

  if (canShowBootExtraText()) {
    tft.fillRect(textX, textY, textW, textH, BG_COLOR);
    tft.setTextColor(GOOD_COLOR, BG_COLOR);
    tft.drawString(BOOT_FINAL_LINE1, textX, textY, 2);
    tft.drawString(BOOT_FINAL_LINE2, textX, textY + 16, 2);
  }

  tft.drawRoundRect(loadBarX, loadBarY, loadBarW, loadBarH, 4, ACCENT_COLOR);
  tft.fillRoundRect(loadBarX + 2, loadBarY + 2, loadBarW - 4, loadBarH - 4, 3, GOOD_COLOR);

  tft.fillRect(12, percentY, SCREEN_W - 24, 16, BG_COLOR);
  tft.setTextColor(GOOD_COLOR, BG_COLOR);
  tft.drawString(BOOT_LOAD_COMPLETE_TEXT, 16, percentY, 2);

  delay(BOOT_COMPLETE_PAUSE_MS);
}
void animateLoadBarToTarget(int& currentFillW, int targetFillW, int loadBarX, int loadBarY, int loadBarH, uint16_t pulseColor, uint16_t durationMs) {
  if (targetFillW <= currentFillW) return;

  const int startFillW = currentFillW;
  const int delta = targetFillW - startFillW;
  const uint16_t frameMs = 12;
  const int frames = max(1, (int)durationMs / frameMs);

  for (int frame = 1; frame <= frames; frame++) {
    int nextFillW = startFillW + (delta * frame) / frames;

    while (currentFillW < nextFillW) {
      currentFillW++;
      int drawX = loadBarX + 2 + currentFillW - 1;
      tft.drawFastVLine(drawX, loadBarY + 2, loadBarH - 4, pulseColor);
      if (currentFillW > 2) {
        tft.drawFastVLine(drawX, loadBarY + 2, loadBarH - 4, TEXT_COLOR);
      }
    }

    delay(frameMs);
  }
}

void drawTwoLineTypewriterText(
  const char* line1,
  const char* line2,
  int x,
  int y,
  int maxWidth,
  uint16_t color,
  int font,
  int delayMs) {
  tft.setTextColor(color, BG_COLOR);

  int len1 = strlen(line1);
  int len2 = strlen(line2);

  char buf1[64];
  char buf2[64];

  int totalSteps = max(len1, len2);

  for (int i = 1; i <= totalSteps; i++) {
    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));

    strncpy(buf1, line1, min(i, len1));
    strncpy(buf2, line2, min(i, len2));

    tft.fillRect(x, y, maxWidth, 34, BG_COLOR);
    tft.drawString(BOOT_PROMPT_PREFIX, x, y, font);
    tft.drawString(buf1, x + 16, y, font);
    tft.drawString(BOOT_SECOND_LINE_PREFIX, x, y + 16, font);
    tft.drawString(buf2, x + 16, y + 16, font);

    delay(delayMs);
  }
}

// =====================================================
// Static UI
// =====================================================
void drawStaticUI() {
  drawHeaderPanel();
  drawHeadingLabels();
  drawBarFrames();
  drawReadoutPanel();
}

void drawHeaderPanel() {
  tft.fillRoundRect(6, headerTop, SCREEN_W - 12, headerH, 6, PANEL_COLOR);
  tft.drawRoundRect(6, headerTop, SCREEN_W - 12, headerH, 6, headerDimState ? DIM_TEXT : ACCENT_COLOR);

  tft.drawFastVLine(16, headerTop + 8, headerH - 16, headerDimState ? DIM_TEXT : ACCENT_COLOR);
  tft.drawFastVLine(SCREEN_W - 17, headerTop + 8, headerH - 16, headerDimState ? DIM_TEXT : ACCENT_COLOR);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TITLE_COLOR, PANEL_COLOR);
  tft.drawCentreString(PANEL_TITLE, SCREEN_W / 2, titleY, pickTitleFont());
}

void drawHeadingLabels() {
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TEXT_COLOR, BG_COLOR);

  for (int i = 0; i < NUM_BARS; i++) {
    int cx = barX[i] + barW / 2;
    tft.drawCentreString(HEADINGS[i], cx, headingY, pickLabelFont());
  }
}

void drawBarFrames() {
  for (int i = 0; i < NUM_BARS; i++) {
    tft.drawRoundRect(barX[i] - 2, barTop - 2, barW + 4, barH + 4, 3, FRAME_COLOR);

    for (int y = barTop + 24; y < barBottom; y += max(28, SCREEN_H / 8)) {
      tft.drawFastHLine(barX[i] + 2, y, barW - 4, PANEL_COLOR);
    }
  }
}

void drawReadoutPanel() {
  tft.fillRoundRect(6, readoutY, SCREEN_W - 12, readoutH, 5, PANEL_COLOR);
  tft.drawRoundRect(6, readoutY, SCREEN_W - 12, readoutH, 5, ACCENT_COLOR);
  updateReadoutPanel();
}

// =====================================================
// Vital logic
// =====================================================
uint8_t vitalState(int idx, int percent) {
  if (percent < BAD_THRESHOLD[idx]) return 2;
  if (percent < WARN_THRESHOLD[idx]) return 1;
  return 0;
}

uint16_t stateColor(uint8_t state) {
  if (state == 2) return BAD_COLOR;
  if (state == 1) return WARN_COLOR;
  return GOOD_COLOR;
}

int oxygenPercentFromTime() {
  unsigned long elapsed = millis() - cycleStartMs;
  if (elapsed >= O2_CYCLE_MS) return 0;

  float p = (float)elapsed / (float)O2_CYCLE_MS;
  float remaining = 1.0f - powf(p, 1.85f);

  int percent = (int)(remaining * 100.0f);
  return constrain(percent, 0, 100);
}

int heightFromPercent(int percent) {
  percent = constrain(percent, 0, 100);
  return (percent * barH) / 100;
}

int vitalHeight(int idx, float t) {
  if (idx == 0) return heightFromPercent(oxygenPercentFromTime());

  float value = 0.0f;

  switch (idx) {
    case 1:
      value = 0.58f + 0.12f * sinf(t * 0.22f + 0.9f) + 0.03f * sinf(t * 0.05f + 2.3f);
      break;
    case 2:
      value = 0.67f + 0.09f * sinf(t * 0.18f + 2.0f) + 0.04f * sinf(t * 0.06f + 0.5f);
      break;
    case 3:
      value = 0.54f + 0.14f * sinf(t * 0.31f + 1.6f) + 0.03f * sinf(t * 0.09f + 2.8f);
      break;
    case 4:
      value = 0.50f + 0.11f * sinf(t * 0.20f + 2.7f) + 0.04f * sinf(t * 0.05f + 1.1f);
      break;
  }

  value = constrain(value, 0.08f, 0.96f);
  return (int)(value * barH);
}

// =====================================================
// Flash logic
// =====================================================
void startRedFlash(int idx) {
  flashActive[idx] = true;
  flashCount[idx] = RED_FLASH_TOGGLES;
  flashVisible[idx] = true;
}

void updateFlashState(int idx) {
  if (!flashActive[idx]) return;

  if (flashCount[idx] > 0) {
    flashVisible[idx] = !flashVisible[idx];
    flashCount[idx]--;
  } else {
    flashActive[idx] = false;
    flashVisible[idx] = false;
  }
}

// =====================================================
// Occasional red event
// =====================================================
void updateOccasionalRedEvent() {
  unsigned long now = millis();
  unsigned long redEventMs = (unsigned long)RANDOM_RED_EVENT_SECONDS * 1000UL;

  if (redEventActive) {
    if (now - redEventStartMs >= redEventMs) {
      redEventActive = false;
      redEventBar = -1;
    }
    return;
  }

  if (now - lastRedEventCheckMs < 3000) return;
  lastRedEventCheckMs = now;

  if (random(100) < RANDOM_RED_EVENT_CHANCE_PERCENT) {
    redEventBar = random(1, NUM_BARS);
    redEventActive = true;
    redEventStartMs = now;
    startRedFlash(redEventBar);
  }
}

bool isRedEventBar(int idx) {
  return redEventActive && idx == redEventBar;
}

// =====================================================
// Segmented bars
// =====================================================
void drawSegmentedBar(int idx, int height, uint16_t color, bool fullFlash) {
  int segStep = segHeight + segGap;
  int totalSegs = max(1, (barH + segGap) / segStep);
  int litSegs = fullFlash ? totalSegs : (height / segStep);

  for (int s = 0; s < totalSegs; s++) {
    int segY = barBottom - segHeight - (s * segStep);
    if (segY < barTop) break;

    uint16_t segColor = (s < litSegs) ? color : OFF_SEG_COLOR;
    tft.fillRoundRect(barX[idx], segY, barW, segHeight, 2, segColor);
  }
}

// =====================================================
// Drawing updates
// =====================================================
void updateBars() {
  for (int i = 0; i < NUM_BARS; i++) {
    int h = vitalHeight(i, animT);
    int percent = (i == 0) ? oxygenPercentFromTime() : (h * 100) / barH;
    uint8_t state = vitalState(i, percent);

    if (i == 0) {
      if (percent <= LOW_OXYGEN_CRITICAL_PERCENT) {
        if (state < 2) state = 2;
      } else if (percent <= LOW_OXYGEN_PREALERT_PERCENT) {
        if (!lowOxygenPreAlertActive) {
          startRedFlash(i);
          lowOxygenPreAlertActive = true;
        }
        if (state < 1) state = 1;
      } else {
        lowOxygenPreAlertActive = false;
      }
    }

    if (isRedEventBar(i)) state = 2;

    if (prevStates[i] != 255 && prevStates[i] != 2 && state == 2) {
      startRedFlash(i);
    }

    updateFlashState(i);

    if (flashActive[i] && flashVisible[i]) {
      drawSegmentedBar(i, h, BAD_COLOR, true);
    } else {
      drawSegmentedBar(i, h, stateColor(state), false);
    }

    prevStates[i] = state;
  }
}

void updateReadoutPanel() {
  char line[40];
  snprintf(line, sizeof(line), "CPU LOAD %3u.%1u%%", cpuLoadEstimateTenths / 10, cpuLoadEstimateTenths % 10);

  tft.fillRect(12, readoutY + 6, SCREEN_W - 24, readoutH - 12, PANEL_COLOR);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TEXT_COLOR, PANEL_COLOR);
  tft.drawCentreString(line, SCREEN_W / 2, readoutY + 8, pickReadoutFont());
}

// =====================================================
// Effects
// =====================================================
void updateEffects() {
  if (SHOW_GLITCH && millis() - lastPixelFlickerMs >= PIXEL_FLICKER_CHECK_MS) {
    lastPixelFlickerMs = millis();
    updatePixelFlicker();
  }

  if (SHOW_GLITCH && millis() - lastGlitchCheckMs >= GLITCH_CHECK_MS) {
    lastGlitchCheckMs = millis();
    if (random(100) < glitchChancePercent) {
      glitchEffect();
    }
  }
}

void updatePixelFlicker() {

  uint8_t burstChancePercent = glitchChancePercent;
  if (random(100) < burstChancePercent) {
    drawPixelFlickerBurst(pixelFlickerCount);
  }
}

void drawPixelFlickerBurst(int count) {
  for (int i = 0; i < count; i++) {
    int x = random(8, SCREEN_W - 8);
    int y = random(headerTop + 4, readoutY + readoutH - 4);

    if (y >= titleY - 8 && y <= titleY + 20) continue;

    uint16_t color;
    int r = random(100);
    if (r < 70) color = DIM_TEXT;
    else if (r < 88) color = TEXT_COLOR;
    else if (r < 94) color = ACCENT_COLOR;
    else color = WARN_COLOR;

    tft.drawPixel(x, y, color);
  }

  delay(12);
  redrawDynamicArea();
}

void glitchEffect() {
  int mode = random(3);

  if (mode == 0) {
    headerDimState = !headerDimState;
    drawHeaderPanel();
    delay(18);
    headerDimState = !headerDimState;
    drawHeaderPanel();
    drawHeadingLabels();
  } else if (mode == 1) {
    int y = random(barTop, barBottom);
    int x = random(0, SCREEN_W / 3);
    int w = random(SCREEN_W / 4, SCREEN_W / 2);
    tft.drawFastHLine(x, y, w, ACCENT_COLOR);
    delay(14);
    redrawDynamicArea();
  } else {
    tft.fillRect(12, readoutY + 6, SCREEN_W - 24, readoutH - 12, BG_COLOR);
    delay(16);
    updateReadoutPanel();
  }
}

void redrawDynamicArea() {
  updateBars();
  updateReadoutPanel();
}
