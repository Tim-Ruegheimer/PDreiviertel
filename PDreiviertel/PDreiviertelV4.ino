
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "HX711.h"

// --- Pins für HX711 ---
#define HX711_DT 10
#define HX711_SCK 11

// --- Pins für Taster ---
#define BALANCE_PIN 12
#define CALIBRATE_PIN 13

// --- Pin für Batterie ---
#define BAT_PIN 4

// --- Display Setup ---
#define GFX_DEV_DEVICE LILYGO_T_DISPLAY_S3_AMOLED
Arduino_DataBus *bus = new Arduino_ESP32QSPI(6, 47, 18, 7, 48, 5);
Arduino_GFX *gfx = new Arduino_RM67162(bus, 17, 3);  // Rotation 1 = Querformat
Arduino_GFX *gfx2;

HX711 scale;

const int movingAvgSize = 15;  // Anzahl der Werte im Moving Average Filter
long values[movingAvgSize];    // Array für die letzten 15 Messwerte
int currentIndex = 0;          // Aktueller Index für den neuen Wert
int sum = 0;
float maxValue = 0.01;
float calibratedOffset = 26406;  //mv/V -0.002290;
float calibratedScalingFactor = 0.00000045938993; //0.00000016
float blancedOffset = 0;

// --- Farben und Design ---
#define BACKGROUND_COLOR BLACK
#define TEXT_COLOR GREEN
#define HIGHLIGHT_COLOR RED

// --- HX711-Kalibrierung ---
#define LOADCELL_REFERENCE_VOLTAGE 2.74  // Versorgung in V
#define AMPLIFIER_GAIN 64.0
#define FULL_SCALE_INPUT_MV 2
#define BOOT_PIN 0



//void calibrateOneProm
/*
void balance(float avRawBalanced) {
 
  calibratedScalingFactor = 0.398 /avRawBalanced; 
 
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

*/

void setBrightness(uint8_t value) {
  bus->beginWrite();
  bus->writeC8D8(0x51, value);
  bus->endWrite();
}

float calculateMvPerV(long raw) {


  Serial.print("calibratedScalingFactor ");
    Serial.println(calibratedScalingFactor,20);

  float mvPerV = raw * calibratedScalingFactor;  // HX711 = 24-bit signed
  
  return mvPerV;
}

double calculateTrimR(float avMvPerV) {

  double uSoll = 0.5 - (avMvPerV / 1000);  //[mV/V] Bei Messbrücke sollten gegenüber BVS- S+ 500 mV/V sein
  double Rtotal = (uSoll * 350) / (1 - uSoll);
  double rTrim = 1.0 / ((1.0 / Rtotal) - (1.0 / 350.0));  //[.= um RUndung]
  rTrim = rTrim / 1000;                                   //[Ohm -> kOhm]
  return rTrim;                                           //[kOhm]
}

float calculatePromil(double TrimR) {

  // --- Widerstandswerte für Promilausgabe ---
  const float promil[19][2] = {
        {1740000, 0.2},
        {874065, 0.4},
        {582923, 0.6},
        {437150, 0.8},
        {349650, 1.0},
        {291316, 1.2},
        {218000, 1.6},
        {194094, 1.8},
        {145483, 2.4},
        {109025, 3.2},
        {96872, 3.6},
        {72566, 4.8},
        {54337, 6.4},
        {48261, 7.2},
        {36108, 9.6},
        {26993, 12.8},
        {23995, 14.4},
        {17879, 19.2},
        {11803, 28.8},
  };


  long minDiff = abs((TrimR*1000) - promil[0][0]);
  float closest = promil[0][1];

  for (int i = 1; i < 19; i++) {
    long diff = abs((TrimR*1000) - promil[i][0]);
    if (diff < minDiff) {
      minDiff = diff;
      closest = promil[i][1];
    }
  }

  return closest;
  

}

//Batteriespannung berechnen

float calculateBat(){
  int rawBat = analogRead(BAT_PIN);
  float voltage = (rawBat * 1.61) / 1000;
  return voltage;
}

void drawValue(float mvPerV, float avMvPerV, float promil, float batVoltage) {


  char buf[20];
  snprintf(buf, sizeof(buf), "%+.6f", avMvPerV);  // Vorzeichen und 6 Dezimalstellen

  // Den Wert in zwei Teile aufteilen
  String fullValue = String(buf);                                     // Der gesamte formatierte Wert als String
  String firstPart = fullValue.substring(0, fullValue.length() - 3);  // Alle bis auf die letzten 3 Ziffern
  String lastPart = fullValue.substring(fullValue.length() - 3);      // Die letzten 3 Ziffern

  gfx2->fillScreen(BACKGROUND_COLOR);

  gfx2->setTextColor(HIGHLIGHT_COLOR);
  gfx2->setTextSize(4, 4);
  gfx2->setCursor(420, 60);  //  gfx2->setCursor(10, 20);
  gfx2->println("mV/V");

  gfx2->setTextColor(TEXT_COLOR);
  gfx2->setTextSize(7, 7);
  gfx2->setTextColor(GREEN);
  gfx2->setCursor(20, 40);
  gfx2->print(firstPart);

  // Den letzten Teil in GrünGrau ausgeben
  uint16_t green = RGB565(0, 160, 0);
  gfx2->setTextColor(green);
  gfx2->print(lastPart);


  int barWidth = 436;
  int barHeight = 25;
  int barX = 50;
  int barY = 5;  //200

  int filled = (int)(mvPerV / maxValue * barWidth);
  if (filled > barWidth) filled = barWidth;

  uint16_t barColor = GREEN;
  if (abs(mvPerV) > maxValue / 2 * 0.8) barColor = RED;
  else if (abs(mvPerV) > maxValue / 2 * 0.6) barColor = YELLOW;

  gfx2->drawRect(barX - 1, barY - 1, barWidth + 2, barHeight + 2, WHITE);
  gfx2->fillRect(barX + barWidth / 2, barY, filled, barHeight, barColor);


  double TrimR = calculateTrimR(avMvPerV);

  gfx2->setTextSize(2, 2);
  gfx2->setTextColor(WHITE);
  gfx2->setCursor(21, 107);
  gfx2->print("Required Trim Resistor");



  gfx2->setTextSize(3, 3);
  gfx2->setTextColor(GREEN);

  gfx2->setCursor(287, 100);


  if (abs(TrimR) > 99999) {
    double TrimRMega = TrimR / 1000;                        //MegaOhm
    snprintf(buf, sizeof(buf), "%+*.*f", 6, 0, TrimRMega);  // Vorzeichen und 6 Dezimalstellen [kOhm]
    gfx2->print(buf);

    gfx2->setCursor(420, 100);
    gfx2->setTextColor(RED);
    gfx2->print("MOhm ");
  }

  else {

    snprintf(buf, sizeof(buf), "%+*.*f", 6, 0, TrimR);  // Vorzeichen und 6 Dezimalstellen [kOhm]
    gfx2->print(buf);

    gfx2->setCursor(420, 100);
    gfx2->setTextColor(RED);
    gfx2->print("kOhm ");
  }

  //Promille Ausgabe
  gfx2->setTextSize(2, 2);
  gfx2->setTextColor(WHITE);
  gfx2->setCursor(215, 140);
  gfx2->print("Promil");

  uint16_t promilColor = RGB565(250, 0, 250);
  gfx2->setTextSize(3, 3);
  gfx2->setTextColor(promilColor);
  gfx2->setCursor(310, 136);
  snprintf(buf, sizeof(buf), "%*.*f", 3, 1, promil);
  gfx2->print(buf);
  
  gfx2->setCursor(420, 136);
  gfx2->setTextColor(promilColor);
  gfx2->print("% ");

  gfx2->setCursor(430, 136);
  gfx2->setTextColor(promilColor);
  gfx2->print(". ");

  //Batteriespannung ausgabe 
  gfx2->setTextSize(2, 2);
  gfx2->setTextColor(WHITE);
  gfx2->setCursor(93, 180);
  gfx2->print("Batteriespannung");

    //Farbe in Abhängigkeit der Spannung ändern 
  uint16_t batColor = RGB565(250, 250, 0);
  if (batVoltage < 3.8) batColor = RED;
  else if (batVoltage > 4) batColor = GREEN;
  
  gfx2->setTextSize(3, 3);
  gfx2->setCursor(310, 176);
  gfx2->setTextColor(batColor);
  snprintf(buf, sizeof(buf), "%*.*f", 2, 2, batVoltage);
  gfx2->print(buf);

  gfx2->setCursor(420, 176);
  gfx2->setTextColor(batColor);
  gfx2->print("V ");



  gfx2->flush();
}

void setup() {
  //Batterie definieren
  pinMode(BAT_PIN, INPUT);

  // Touch-Version: Display aktivieren
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);
  pinMode(BOOT_PIN, INPUT_PULLUP);  // Setze GPIO0 als Eingang mit Pull-Up-Widerstand


  Serial.begin(115200);
  //delay(1000);

  for (int i = 0; i < movingAvgSize; i++) {
    values[i] = 0;
  }

  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }

  gfx2 = new Arduino_Canvas(536, 240, gfx, 0, 0);
  gfx2->begin(GFX_SKIP_OUTPUT_BEGIN);
  gfx2->fillScreen(BACKGROUND_COLOR);
  gfx2->flush();

  setBrightness(150);

  // Bootup Logo

  // HX711 Setup
  scale.begin(HX711_DT, HX711_SCK);
  scale.set_gain(128);  // Verstärkung: 128, 64 oder 32
  Serial.println("HX711 gestartet...");

  if (!scale.is_ready()) {
    Serial.println("HX711 nicht bereit!");
  } else {
    Serial.println("HX711 bereit.");
  }

  //Taster  
  pinMode(BALANCE_PIN, INPUT_PULLUP);
}


void loop() {

  if (scale.is_ready()) {
    long raw = scale.read();

    values[currentIndex++] = raw;
    if (currentIndex >= movingAvgSize) currentIndex = 0;

    long sum = 0;
    for (int i = 0; i < movingAvgSize; i++) {
      sum += values[i];
    }

    long avRaw = sum / movingAvgSize;
    
    Serial.print("avRaw");
    Serial.println(avRaw);


    float mvPerV = calculateMvPerV(raw - calibratedOffset - blancedOffset);
    float avMvPerV = calculateMvPerV(avRaw - calibratedOffset - blancedOffset);

    //Serial.println(avRaw);
    //Serial.println(avMvPerV);

    double TrimR = calculateTrimR(avMvPerV);
    float promil = calculatePromil(abs(TrimR));
    float batVoltage = calculateBat();
   


    if (!digitalRead(BALANCE_PIN)) {
      balance(avRaw - calibratedOffset);
    }


    if (maxValue / 2 < abs(avMvPerV)) {  //2 wegen beiden Bereichen Positiv und Negativ
      maxValue = abs(avMvPerV) * 2;
      Serial.println("updateMaxVal");
    }

    drawValue(mvPerV, avMvPerV, promil, batVoltage);
    // Serial.printf("Raw: %ld -> %.6f mV/V\n", raw, mvPerV);
  }


  else {
    // Serial.println("Warte auf HX711...");
  }
}