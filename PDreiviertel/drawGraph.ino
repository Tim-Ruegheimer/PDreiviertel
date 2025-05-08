void drawGraph(float mvPerV, float avMvPerV, float promil, float batVoltage) {
 int graphHeight = 100;

  gfx2->fillScreen(BACKGROUND_COLOR);
 gfx2->drawPixel(50, 50, HIGHLIGHT_COLOR);


 int x = storageSize;
 while( x > 0){                                  // übers Display Iterieren 
  int pos =   currentIndex - (storageSize-x);      //array position
  if(pos < 0){                               //array wrap around
    pos = storageSize - pos;
  }
  
  int raw = values[pos];
  Serial.print("pos: ");
  Serial.println(pos);
  float mvPerV = calculateMvPerV(raw - calibratedOffset - blancedOffset);
  int y = (int)(mvPerV / maxValue * graphHeight);


  gfx2->drawPixel(x, y + displayHeight/2, HIGHLIGHT_COLOR);
  x--;
 }

/*
  for(int i = (storageSize-1); i > 0; i-- ){
    int x = currentIndex - i;
    if (x < 0){
      stora
    }
  }
*/

  gfx2->flush();
}