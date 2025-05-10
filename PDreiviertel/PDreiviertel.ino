#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "HX711.h"

// --- Pins für HX711 ---
#define HX711_DT 10
#define HX711_SCK 11

// --- Pins für Taster ---
#define BALANCE_PIN 12
#define MODE_PIN 13

// --- Pin für ADC der Batterie ---
#define BAT_PIN 4

// --- Display Setup ---
#define GFX_DEV_DEVICE LILYGO_T_DISPLAY_S3_AMOLED
Arduino_DataBus *bus = new Arduino_ESP32QSPI(6, 47, 18, 7, 48, 5);
Arduino_GFX *gfx = new Arduino_RM67162(bus, 17, 3);  // Rotation 1 = Querformat
Arduino_GFX *gfx2;

HX711 scale;

const int storageSize = 536;        // As many pixels we have
const int displayHeight = 240;
const int displayWidth = 536;

long values[storageSize];
int currentIndex = 0;
float maxValue = 0.002; /// NOch ein define bauen!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


float calibratedOffset = 2638;
float calibratedScalingFactor = 0.00000045855;
float blancedOffset = 0;
int modeNbr = 1;

// --- Farben und Design ---
#define BACKGROUND_COLOR BLACK
#define TEXT_COLOR GREEN
#define HIGHLIGHT_COLOR RED

#define LOADCELL_REFERENCE_VOLTAGE 2.74
#define AMPLIFIER_GAIN 64.0
#define FULL_SCALE_INPUT_MV 2
#define BOOT_PIN 0

unsigned long lastSampleTime = 0;
const unsigned long sampleInterval = 1000 / 80;  // 80 SPS ≈ 12 ms
unsigned long lastDrawTime = 0;
const unsigned long drawInterval = 200;          // Display alle 200 ms

void setBrightness(uint8_t value) {
  bus->beginWrite();
  bus->writeC8D8(0x51, value);
  bus->endWrite();
}

float calculateMvPerV(long raw) {
  return raw * calibratedScalingFactor;
}

double calculateTrimR(float avMvPerV) {
  double uSoll = 0.5 - (avMvPerV / 1000);
  double Rtotal = (uSoll * 350) / (1 - uSoll);
  double rTrim = 1.0 / ((1.0 / Rtotal) - (1.0 / 350.0));
  return rTrim / 1000;
}

float calculatePromil(double TrimR) {
  const float promil[19][2] = {
    {1740000, 0.2}, {874065, 0.4}, {582923, 0.6}, {437150, 0.8},
    {349650, 1.0}, {291316, 1.2}, {218000, 1.6}, {194094, 1.8},
    {145483, 2.4}, {109025, 3.2}, {96872, 3.6}, {72566, 4.8},
    {54337, 6.4}, {48261, 7.2}, {36108, 9.6}, {26993, 12.8},
    {23995, 14.4}, {17879, 19.2}, {11803, 28.8},
  };
  long minDiff = abs((TrimR * 1000) - promil[0][0]);
  float closest = promil[0][1];
  for (int i = 1; i < 19; i++) {
    long diff = abs((TrimR * 1000) - promil[i][0]);
    if (diff < minDiff) {
      minDiff = diff;
      closest = promil[i][1];
    }
  }
  return closest;
}

float calculateBat() {
  int rawBat = analogRead(BAT_PIN);
  return (rawBat * 1.61) / 1000;
}

float calculateTemp(float avMvPerV) {
  if (avMvPerV > 0) {
    double UPt100 = 100.0 / 22100.0;
    double zaehler = (UPt100 + (avMvPerV / 1000.0)) * 22000.0;
    double nenner = 1.0 - UPt100 + (avMvPerV / 1000.0);
    double resistancePt100 = zaehler / nenner;
    double temp = (resistancePt100 - 100) / (100 * 0.00391);
    return temp;
  }
  return 0;
}



void balance(long avRawBalanced) {
  calibratedScalingFactor = 0.398 / avRawBalanced;
  gfx2->fillScreen(BLACK);
  gfx2->setTextColor(ORANGE);
  gfx2->setTextSize(8, 8);
  gfx2->setCursor(50, 80);
  gfx2->println("Balanced");
  gfx2->flush();
  while (digitalRead(BOOT_PIN) == LOW) {
    delay(50);
  }
}

void setup() {
  pinMode(BAT_PIN, INPUT);
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);
  pinMode(BOOT_PIN, INPUT_PULLUP);

  Serial.begin(115200);

  for (int i = 0; i < storageSize; i++) values[i] = 0;

  if (!gfx->begin()) Serial.println("gfx->begin() failed!");
  gfx2 = new Arduino_Canvas(displayWidth, displayHeight, gfx, 0, 0);
  gfx2->begin(GFX_SKIP_OUTPUT_BEGIN);
  gfx2->fillScreen(BACKGROUND_COLOR);
  gfx2->flush();
  setBrightness(150);

  scale.begin(HX711_DT, HX711_SCK);
  scale.set_gain(128);
  Serial.println(scale.is_ready() ? "HX711 bereit." : "HX711 nicht bereit!");

  pinMode(BALANCE_PIN, INPUT_PULLUP);
  pinMode(MODE_PIN, INPUT_PULLUP);
}

void loop() {
  unsigned long now = millis();

  if (now - lastSampleTime >= sampleInterval && scale.is_ready()) {
    lastSampleTime = now;
    long raw = scale.read();
    values[currentIndex] = raw;
    currentIndex = (currentIndex + 1) % storageSize;
  }

  if (now - lastDrawTime >= drawInterval) {
    lastDrawTime = now;

    long avRaw = getFilteredAverage();
    float mvPerV = calculateMvPerV(values[(currentIndex - 1 + storageSize) % storageSize] - calibratedOffset - blancedOffset);
    float avMvPerV = calculateMvPerV(avRaw - calibratedOffset - blancedOffset);
    double TrimR = calculateTrimR(avMvPerV);
    float promil = calculatePromil(abs(TrimR));
    float batVoltage = calculateBat();

    if (!digitalRead(BALANCE_PIN)) balance(avRaw - calibratedOffset);
    if (!digitalRead(MODE_PIN)) mode();

    if (maxValue / 2 < abs(avMvPerV)) {
      maxValue = abs(avMvPerV) * 2;
      Serial.println("updateMaxVal");
    }

    switch (modeNbr) {
      case 1:
        drawOneValue(mvPerV, avMvPerV, promil, batVoltage);
        break;
      case 2:
        drawGraph(mvPerV, avMvPerV, promil, batVoltage);
        break;
      default:
        drawOneValue(mvPerV, avMvPerV, promil, batVoltage);
        break;
    }
  }
}
