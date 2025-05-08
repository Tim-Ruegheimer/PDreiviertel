
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

const int movingAvgSize = 10;  // Anzahl der Werte im Moving Average Filter
const int storageSize = 536;        // As many pixels we have
const int displayHeight = 240;
long values[storageSize];    // Array für die letzten 15 Messwerte
int currentIndex = 0;          // Aktueller Index für den neuen Wert
int sum = 0;
float maxValue = 0.01;
float calibratedOffset = 2638;  //mv/V -0.002290;
float calibratedScalingFactor = 0.00000045855; //0.00000016
float blancedOffset = 0;
int modeNbr = 1;

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

float calculateTemp(float avMvPerV){
  if(avMvPerV > 0){
    double UPt100 = 100.0/22100.0; // 100 Ohm referenzwiderstand und 22K drüber Common mode spannung an wheatstone brücke
    double zaehler = (UPt100 + (avMvPerV/1000.0)) * 22000.0;
    double nenner = 1.0 - UPt100 + (avMvPerV/1000.0);
     double resistancePt100 = ( zaehler) / (nenner);  //U2*R1 / Uges - U2

   
    double A = 0.0039083;
    double B = -5.775*pow(10, -7);

    Serial.print("avMvPerV ");
    Serial.println(avMvPerV,20);

    
    Serial.print("UPt100 ");
    Serial.println(UPt100,20);

    Serial.print("zaehler ");
    Serial.println(zaehler,20);

    
    Serial.print("nenner ");
    Serial.println(nenner,20);

    Serial.print("resistance ");
    Serial.println(resistancePt100,20);

   // double temp = (-A*100 + sqrt((A*100*A*100)- 4*B*100* (100-resistancePt100))) / (2*B*100);
     double temp = (resistancePt100-100) / (100 * 0.00391);

    Serial.print("temp ");
    Serial.println(temp,20);

    return temp;
    
  }


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

  for (int i = 0; i < storageSize; i++) {
    values[i] = 0;
  }

  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }


  gfx2 = new Arduino_Canvas(536, displayHeight, gfx, 0, 0);
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
  pinMode(MODE_PIN, INPUT_PULLUP);
}


void loop() {


  if (scale.is_ready()) {
    long raw = scale.read();

    values[currentIndex++] = raw;
    if (currentIndex >= storageSize) currentIndex = 0;

    long sum = 0;
    for (int i = 0; i < storageSize; i++) {
      sum += values[i];
    }

    long avRaw = sum / storageSize;
    
    Serial.print("avRaw");
    Serial.println(avRaw);


    float mvPerV = calculateMvPerV(raw - calibratedOffset - blancedOffset);
    float avMvPerV = calculateMvPerV(avRaw - calibratedOffset - blancedOffset);

    //Serial.println(avRaw);
    //Serial.println(avMvPerV);

    double TrimR = calculateTrimR(avMvPerV);
    calculateTemp(avMvPerV);
    float promil = calculatePromil(abs(TrimR));
    float batVoltage = calculateBat();
   


    if (!digitalRead(BALANCE_PIN)) {
      balance(avRaw - calibratedOffset);
    }

    if (!digitalRead(MODE_PIN)) {
      mode();
    }



    if (maxValue / 2 < abs(avMvPerV)) {  //2 wegen beiden Bereichen Positiv und Negativ
      maxValue = abs(avMvPerV) * 2;
      Serial.println("updateMaxVal");
    }

    switch (modeNbr) {
      case 1:
         drawOneValue(mvPerV, avMvPerV, promil, batVoltage);
        break;
      case 2:
       drawGraph( mvPerV, avMvPerV,  promil, batVoltage);
        break;
      default:
         drawOneValue(mvPerV, avMvPerV, promil, batVoltage);
        break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
    }
   
    // Serial.printf("Raw: %ld -> %.6f mV/V\n", raw, mvPerV);
  }


  else {
    // Serial.println("Warte auf HX711...");
  }
}