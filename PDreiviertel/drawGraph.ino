void drawGraph(float mvPerV, float avMvPerV, float promil, float batVoltage) {
  int graphHeight = 100;               // Höhe des Graphen
  int centerY = displayHeight / 2;     // Zentrale Y-Position (Null-Linie)
  int lastY = centerY;                 // Startwert für die vorherige Y-Position

  gfx2->fillScreen(BACKGROUND_COLOR);  // Hintergrund löschen

  // Überschrift "ch1 Graph" zeichnen
  gfx2->setTextColor(HIGHLIGHT_COLOR);
  gfx2->setTextSize(3);                // Textgröße anpassen
  gfx2->setCursor(2, 2);               // Position
  gfx2->print("ch1 RollingGraph");

  // Nulllinie (Mittelachse) zeichnen
  gfx2->drawLine(0, centerY, displayWidth - 1, centerY, LIGHTGREY);

  // 50% Ausschlag-Linien zeichnen (positive und negative Linie)
  int fiftyPercentOffset = (int)(graphHeight * 0.25); // 50% vom maximalen Ausschlag
  int y50p = centerY - fiftyPercentOffset;
  int yMinus50p = centerY + fiftyPercentOffset;

  gfx2->drawLine(0, y50p, displayWidth - 1, y50p, YELLOW);  // Positive 50%-Linie
  gfx2->drawLine(0, yMinus50p, displayWidth - 1, yMinus50p, YELLOW);  // Negative 50%-Linie

  // Rechteck um den Graphen zeichnen (Ränder)
  int graphMargin = 10;
  gfx2->drawRect(graphMargin, centerY - graphHeight / 2 - graphMargin, displayWidth - graphMargin * 2, graphHeight + graphMargin * 2, WHITE);

  // Start bei der letzten Position im Array
  int x = storageSize - 1;
  while (x >= 0) {
    int pos = (currentIndex - (storageSize - 1 - x) + storageSize) % storageSize;

    // Rohwert aus dem Array holen
    int raw = values[pos];
    float mvPerV = calculateMvPerV(raw - calibratedOffset - blancedOffset);  // Berechne den Wert

    // Um die Mitte (Null-Linie) skalieren, gleiche Skalierung wie im Bargraph
    int yOffset = (int)(mvPerV / maxValue * graphHeight);  // Umrechnung des mvPerV in Y-Offset
    int y = centerY - yOffset;  // Y-Wert anpassen, sodass die Null-Linie in der Mitte bleibt
    y = constrain(y, centerY - graphHeight / 2, centerY + graphHeight / 2);  // Begrenzung des Y-Werts innerhalb des Graphenbereichs

    // Nur Linien zwischen den Punkten zeichnen
    if (x != storageSize - 1) {
      gfx2->drawLine(x + 1, lastY, x, y, HIGHLIGHT_COLOR);  // Zeige Linie vom letzten Punkt zur aktuellen Position
    }

    lastY = y;  // Aktuellen Y-Wert speichern
    x--;  // Zur nächsten X-Position bewegen
  }

  gfx2->flush();  // Alles an den Bildschirm senden
}
