
// Culled Example 

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

#define my_dc   8
#define my_cs   9
#define my_rst  7

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF
 
Adafruit_SSD1351 tft = Adafruit_SSD1351(my_cs, my_dc, my_rst);

unsigned long startTime, endTime; // For calculating FPS.
unsigned int frames;

void setup(void) {
  Serial.begin(9600);

  pinMode(13, INPUT_PULLUP); 
  tft.begin();

  Serial.print("Setup complete");
}

void loop() {

  cls(BLACK);

  // Full screen rectangles:

  startTime = millis();
  frames = 0;

  for (int n = 0; n < 5; n++) { 

    cls(RED); 
    cls(GREEN);
    cls(BLUE);

    frames = frames + 3;

  }

  endTime = millis();
  showFPS("Full screen rects:");

  cls(BLACK);

  // Draw rectangles of random color and size:
  for (int n = 0; n<200; n++) {
    byte c = random(0, 3);
    uint16_t color = BLACK;
    if (c == 0) { color = RED; }
    if (c == 1) { color = GREEN; }
    if (c == 2) { color = BLUE; } 
    fastRect(random(0, 128), random(0, 128), random(0, 128), random(0, 128), color);
  }    
  tft.draw();

  cls(BLACK);
  testdrawtext("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a tortor imperdiet posuere.", WHITE);
  delay(500);

}

