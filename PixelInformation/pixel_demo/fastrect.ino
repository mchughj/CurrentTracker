/*
Use tft.Color565(r,g,b) or the #defines above (BLACK, WHITE, BLUE, etc) for color.

This function could probably be made faster if I were to access the SPI bus directly.
*/
void fastRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {

  int16_t tmp;
  
  // Swap coordinates if necessary, so x1 and y1 are the smaller of the two.
    if (x2 < x1) { tmp = x1; x1 = x2; x2 = tmp; } 
    if (y2 < y1) { tmp = y1; y1 = y2; y2 = tmp; }
  
  // Clip rect to screen.

    if (x1 > 127 ) { return; }
    if (y1 > 127 ) { return; }
    if (x2 < 0) { return; } 
    if (y2 < 0) { return; }
  
    if (x1 < 0) { x1 = 0; }
    if (y1 < 0) { y1 = 0; }
    if (x2 > 127) { x2 = 127; }
    if (y2 > 127) { y2 = 127; }
  
  tft.writeCommand(SSD1351_CMD_SETCOLUMN);
  tft.write16(x1);
  tft.write16(x2); 
  
  tft.writeCommand(SSD1351_CMD_SETROW);
  tft.write16(y1);
  tft.write16(y2);

  tft.writeCommand(SSD1351_CMD_WRITERAM); // Tell display we're going to send it image data in a moment. (Not sure if necessary.) 
  digitalWrite(my_dc, HIGH); // Set DATA/COMMAND pin to DATA.    

  digitalWrite(my_cs, LOW); // Tell display to pay attention to the incoming data.
    
  uint16_t pixels = (x2-x1 + 1) * (y2-y1 + 1);

  SPI.beginTransaction(SPISettings(12000000, MSBFIRST, SPI_MODE0)); 
  for (int n = 0; n < pixels; n++) { 
    //SPI.write(color>>8);  
    //SPI.write(color);  
    SPI.transfer16(color); // This function is much slower than a properly written SPI write command.          
  } 
  SPI.endTransaction();
      
  digitalWrite(my_cs, HIGH); // Tell display we're done talking to it for now, so the next SD read doesn't corrupt the screen.
   
}  
