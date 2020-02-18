void testlines(uint16_t color1, uint16_t color2) {
   cls(BLACK);
   for (uint16_t x=0; x < tft.width()-1; x+=6) {
     tft.drawLine(0, 0, x, tft.height()-1, color1);
   }
   for (uint16_t y=0; y < tft.height()-1; y+=6) {
     tft.drawLine(0, 0, tft.width()-1, y, color1);
   }
   
   //cls(BLACK);
   for (uint16_t x=0; x < tft.width()-1; x+=6) {
     tft.drawLine(tft.width()-1, 0, x, tft.height()-1, color2);
   }
   for (uint16_t y=0; y < tft.height()-1; y+=6) {
     tft.drawLine(tft.width()-1, 0, 0, y, color2);
   }
   
   cls(BLACK);
   for (uint16_t x=0; x < tft.width()-1; x+=6) {
     tft.drawLine(0, tft.height()-1, x, 0, color1);
   }
   for (uint16_t y=0; y < tft.height()-1; y+=6) {
     tft.drawLine(0, tft.height()-1, tft.width()-1, y, color1);
   }

   //cls(BLACK);
   for (uint16_t x=0; x < tft.width()-1; x+=6) {
     tft.drawLine(tft.width()-1, tft.height()-1, x, 0, color2);
   }
   for (uint16_t y=0; y < tft.height()-1; y+=6) {
     tft.drawLine(tft.width()-1, tft.height()-1, 0, y, color2);
   }
   
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0,0);
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.print(text);
}

void testfastlines(uint16_t color1, uint16_t color2) {
   cls(BLACK);
   for (uint16_t y=0; y < tft.height()-1; y+=5) {
     tft.drawFastHLine(0, y, tft.width()-1, color1);
   }
   for (uint16_t x=0; x < tft.width()-1; x+=5) {
     tft.drawFastVLine(x, 0, tft.height()-1, color2);
   }
}

void testdrawrects(uint16_t color) {
 cls(BLACK);
 for (uint16_t x=0; x < tft.height()-1; x+=6) {
   tft.drawRect((tft.width()-1)/2 -x/2, (tft.height()-1)/2 -x/2 , x, x, color);
 }
}

void testfillrects(uint16_t color1, uint16_t color2) {
 cls(BLACK);
 for (uint16_t x=tft.height()-1; x > 6; x-=6) {
   tft.fillRect((tft.width()-1)/2 -x/2, (tft.height()-1)/2 -x/2 , x, x, color1);
   tft.drawRect((tft.width()-1)/2 -x/2, (tft.height()-1)/2 -x/2 , x, x, color2);
 }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (uint8_t x=radius; x < tft.width()-1; x+=radius*2) {
    for (uint8_t y=radius; y < tft.height()-1; y+=radius*2) {
      tft.fillCircle(x, y, radius, color);
    }
  }  
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (uint8_t x=0; x < tft.width()-1+radius; x+=radius*2) {
    for (uint8_t y=0; y < tft.height()-1+radius; y+=radius*2) {
      tft.drawCircle(x, y, radius, color);
    }
  }  
}

void testtriangles() {
  cls(BLACK);
  int color = 0xF800;
  int t;
  int w = tft.width()/2;
  int x = tft.height();
  int y = 0;
  int z = tft.width();
  for(t = 0 ; t <= 15; t+=1) {
    tft.drawTriangle(w, y, y, x, z, x, color);
    x-=4;
    y+=4;
    z-=4;
    color+=100;
  }
}

void testroundrects() {
  cls(BLACK);
  int color = 100;
  
  int x = 0;
  int y = 0;
  int w = tft.width();
  int h = tft.height();
  for(int i = 0 ; i <= 24; i++) {
    tft.drawRoundRect(x, y, w, h, 5, color);
    x+=2;
    y+=3;
    w-=4;
    h-=6;
    color+=1100;
    //Serial.println(i);
  }
}

void tftPrintTest() {
  cls(BLACK);
  tft.setCursor(0, 5);
  
  tft.setTextColor(RED);  
  tft.setTextSize(1);
  tft.println("Rabid Prototypes");

  tft.println("");
  
  tft.setTextColor(BLUE);
  tft.setTextSize(3);
  tft.println("PIXEL");

  tft.println("");
  
  tft.setTextColor(WHITE);
  tft.setTextSize(1);
  tft.print("32-bit/48MHz");
  
  delay(1500);
}

void tftPrintTest1() {
  
  cls(BLACK);
  
  tft.setTextSize(3);

  // Characters are 6x8 * size, so font size 3 has characters that are 18x24 pixels.
  // PIXEL is therefore 90 pixels wide, and (128-90) / 2 = 19, so we need a margin or rather an X offset of 19 pixels to center the text.
   
  for (int y = 0; y < 128-24; y+=6) { // Stop 24 pixels from bottom so text doesn't scroll off screen.

    int n = (y/3) % 7;
    
    if (n == 0) { tft.setTextColor(tft.color565(255,0,0)); } // Red
    if (n == 1) { tft.setTextColor(tft.color565(255,127,0)); } // Orange
    if (n == 2) { tft.setTextColor(tft.color565(255,255,0)); } // Yellow
    if (n == 3) { tft.setTextColor(tft.color565(0,255,0)); } // Green
    if (n == 4) { tft.setTextColor(tft.color565(0,255,255)); } // Cyan
    if (n == 5) { tft.setTextColor(tft.color565(0,0,255)); } // Blue
    if (n == 6) { tft.setTextColor(tft.color565(127,0,255)); } // Violet

    cls(BLACK);
    tft.setCursor(19, y);
    tft.println("PIXEL");
        
  }
  
}


void tftPrintTest2() { 
  tft.setCursor(0, 5);
  cls(BLACK);
  tft.setTextColor(WHITE);
  tft.setTextSize(0);
  tft.println("Hello World!");  
  tft.setTextSize(1);
  tft.setTextColor(GREEN);
  tft.print(PI, 6);
  tft.println(" Want pi?");
  tft.println(" ");
  tft.print(8675309, HEX); // print 8,675,309 out in HEX!
  tft.println(" Print HEX!");
  tft.println(" ");
  tft.setTextColor(WHITE);
  tft.println("Sketch has been");
  tft.println("running for: ");
  tft.setTextColor(RED);
  tft.print(millis() / 1000);
  tft.setTextColor(WHITE);
  tft.print(" seconds.");
}


/**************************************************************************/
/*! 
    @brief  Renders a simple test pattern on the LCD
*/
/**************************************************************************/
void lcdTestPattern(void)
{
  uint32_t i,j;
  tft.setCursor(0, 0);
  
  for(i=0;i<128;i++)
  {
    for(j=0;j<128;j++)
    {
      if(i<16){
        tft.write16(RED>>8); tft.write16(RED);
      }
      else if(i<32) {
        tft.write16(YELLOW>>8);tft.write16(YELLOW);
      }
      else if(i<48){tft.write16(GREEN>>8);tft.write16(GREEN);}
      else if(i<64){tft.write16(CYAN>>8);tft.write16(CYAN);}
      else if(i<80){tft.write16(BLUE>>8);tft.write16(BLUE);}
      else if(i<96){tft.write16(MAGENTA>>8);tft.write16(MAGENTA);}
      else if(i<112){tft.write16(BLACK>>8);tft.write16(BLACK);}
      else {
        tft.write16(WHITE>>8);      
        tft.write16(WHITE);
       }
    }
  }
}
