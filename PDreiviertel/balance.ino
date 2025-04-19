void balance(float avMvPerV) {
  blancedOffset = avMvPerV;
  maxValue = 0.01;

  gfx2->fillScreen(BLACK);

  gfx2->setTextColor(ORANGE);
  gfx2->setTextSize(8, 8);
  gfx2->setCursor(50, 80);
  gfx2->println("Balanced");
  gfx2->flush();

  while (digitalRead(BALANCE_PIN) == LOW) {
    delay(50);
  }
}