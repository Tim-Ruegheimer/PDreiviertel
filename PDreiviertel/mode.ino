void mode() {
  modeNbr++;

  gfx2->fillScreen(BLACK);

  gfx2->setTextColor(YELLOW);
  gfx2->setTextSize(8, 8);
  gfx2->setCursor(100, 80);
  gfx2->print("MODE ");
  gfx2->println(modeNbr);
  gfx2->flush();

  int i = 0;
  while (digitalRead(MODE_PIN) == LOW) {
    delay(25);
    i++;
    if(i > 30){
      modeNbr = 1;
     gfx2->fillScreen(BLACK);
    gfx2->setTextColor(GREEN);
    gfx2->setTextSize(8, 8);
    gfx2->setCursor(30, 80);
    gfx2->print("MODE RESET");
    gfx2->flush();
    i = 0;
    }
    
  }
}