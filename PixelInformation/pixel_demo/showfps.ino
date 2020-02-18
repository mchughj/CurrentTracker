
void showFPS(char* title) {

  unsigned int ms = endTime-startTime;
  float seconds = (endTime-startTime) / 1000.0;
  float fps = frames / seconds; 
  
  cls(BLACK); // tft.fillScreen(BLACK); <- too slow!
  tft.setCursor(0, 5);

  tft.setTextColor(WHITE);  
  tft.setTextSize(1);  
  tft.println(title);
  tft.println();
   
  tft.setTextColor(RED);  
  tft.setTextSize(1);
  tft.print(frames);
  tft.println(F(" frames in "));

  tft.print(seconds);
  tft.println(F(" seconds"));  
  
  tft.println();
  
  tft.setTextColor(GREEN);
  tft.setTextSize(2);
  tft.print(fps);
  tft.println(F(" FPS"));
   
  delay(3000);
}
