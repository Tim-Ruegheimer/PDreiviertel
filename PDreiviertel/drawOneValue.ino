void drawOneValue(float mvPerV, float avMvPerV, float promil, float batVoltage) {


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
  gfx2->print("BatVoltage");

    //Farbe in Abhängigkeit der Spannung ändern 
  uint16_t batColor = RGB565(250, 250, 0);
  if (batVoltage < 3.6) batColor = RED;
  else if (batVoltage > 3.65) batColor = GREEN;
  
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