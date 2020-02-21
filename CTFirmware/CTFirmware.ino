
//
// This project uses a Pixel 2.0 (https://rabidprototypes.com/product/pixel/)
// and a Coulomb counter (https://www.sparkfun.com/products/12052) in order to 
// track the current used by a project.  
//
// When programming this ensure that the board selected is 
//   "Arduino Zero (Native USB Port)"
//

// Required libraries:
//  * Adafruit_SSD1351 - Version 1.0.1
//  * Adafruit GFX


#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <stdarg.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128 

#define COULOMB_INTERRUPT_PIN 3

#define SCREEN_DC   8
#define SCREEN_CS   9
#define SCREEN_RST  7

Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_CS, SCREEN_DC, SCREEN_RST);

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

// Largest unsigned long int which is a 4 byte integer.
#define ULONG_MAX 0xFFFFFFFF

// The Arduino Zero has two serial ports.  To get output for debugging I use
// the SerialUSB rather than Serial.
#define Serial SerialUSB

// The Arduino zero has an ARM Cortex M0+ processor with 256KB of flash (and a
// pretty hefty 32KB of SRAM).  Not having printf in the core is a little
// silly and my debug information in the below is non-trivial.  So I've
// defined my on printf equivalent. 
void myPrintf(const char * format, ...)
{
  char buf[256];
  int len;

  va_list ap;
  va_start(ap, format);

  len = vsnprintf(buf, 256, format, ap);
  Serial.print(buf);

  va_end(ap);
}

class DurationAdaptiveAverageTracker {
public:
  // Allocate the duration adaptive tracker with the specified number of data
  // elements to track.  Each element takes 5 bytes of space.
  DurationAdaptiveAverageTracker(int numberToTrack);

  void addData(unsigned long int timeMicros, uint8_t numberOfInterrupts);

  // This method will return -1 if there is no good data to support 
  double getApproximateAverage( unsigned long int currentMicros, unsigned long int microDuration );
  double getOverallAverage();
 
private:
  unsigned long int getElapsed( unsigned long int earlierMicros, unsigned long int laterMicros);

  typedef struct {
    unsigned long int timeMicros;
    uint8_t numberOfInterrupts;
  } Datapoint;

  // Circular dynamically allocated array
  Datapoint *d;
  int allocatedDatapoints;
  int nextIndex;
  int numberDatapoints;

  unsigned long int lastMicrosSeen;

  double totalSeconds;
  unsigned long int totalInterrupts;
};


DurationAdaptiveAverageTracker::DurationAdaptiveAverageTracker(int numberToTrack) { 
  allocatedDatapoints = numberToTrack;
  numberDatapoints = 0;
  nextIndex = 0;
  d = new Datapoint[allocatedDatapoints];
  memset( d, 0, sizeof(Datapoint)*allocatedDatapoints);
  lastMicrosSeen = 0;
  totalSeconds = 0;
}


void DurationAdaptiveAverageTracker::addData(unsigned long int timeMicros, uint8_t numberOfInterrupts) {
  if (lastMicrosSeen != 0) { 
    unsigned long int microsElapsed = getElapsed(lastMicrosSeen, timeMicros);
    totalSeconds += ((double)microsElapsed) / 1000000.0;
  }
  totalInterrupts += numberOfInterrupts;
  lastMicrosSeen = timeMicros;
  myPrintf( "addData - updated overall;  timeMicros: %lu, numberOfInterrupts: %d, "
    "totalInterrupts: %d, totalSeconds: %d\n", 
    timeMicros, numberOfInterrupts, totalInterrupts, (int)totalSeconds);

  if (numberDatapoints < allocatedDatapoints) { 
    numberDatapoints++;
  }
  d[nextIndex].timeMicros = timeMicros;
  d[nextIndex].numberOfInterrupts = numberOfInterrupts;

  myPrintf( "addData - done;  timeMicros: %lu, numberOfInterrupts: %d, index: %d, numberDatapoints: %d\n", 
    timeMicros, numberOfInterrupts, nextIndex, numberDatapoints);

  // nextIndex always points one past where the last datapoint was added.  In
  // other words it always is the next place where a new datapoint will be
  // added.
  nextIndex++;
  if (nextIndex == allocatedDatapoints) { 
    nextIndex = 0;
  }
}

double
DurationAdaptiveAverageTracker::getApproximateAverage( unsigned long int currentMicros, unsigned long int microDuration ) {
  unsigned long int maxPastTimeMicros = 0;
  uint8_t interruptsAccumulated = 0;

  myPrintf( "getApproximateAverage - beginning; currentMicros: %lu, "
            "microDuration: %lu, numberDatapoints: %d\n", 
            currentMicros, microDuration, numberDatapoints);

  int index = nextIndex;
  int dataPointCount = 0;
  for (int i=0; i<numberDatapoints; i++) { 
    index = index - 1;
    if (index < 0) { 
      index = allocatedDatapoints - 1;
    }
    unsigned long int elapsedMicros = getElapsed(d[index].timeMicros, currentMicros);
    myPrintf( "getApproximateAverage - examining item; index: %d, "
                   "currentMicros: %lu, itemTimeMicros: %lu, elapsed: %lu\n",
                   index, currentMicros, d[index].timeMicros, elapsedMicros);
    if (elapsedMicros < microDuration) { 
      maxPastTimeMicros = d[index].timeMicros;
      interruptsAccumulated += d[index].numberOfInterrupts;
      dataPointCount++;
    } else {
      break;
    }
  }
  myPrintf( "getApproximateAverage - found index;  currentMicros: %lu, "
                 "microDuration: %lu, nextIndex: %d, index: %d, dataPointCount: %d, "
                 "maxPastTimeMicros: %lu, interruptsAccumulated:%d\n", 
                 currentMicros, microDuration, nextIndex, index,
                 dataPointCount, maxPastTimeMicros, interruptsAccumulated);
  if (dataPointCount == 0) { 
    myPrintf( "getApproximateAverage - dataPointCount is zero;\n");
    return -1;
  }

  // Get the time distance between the currentMicros and the oldest one 
  // that we found. If this is not sufficiently close to the duration 
  // that I am looking for then don't compute the average. 
  // 
  // This does create the behavior where if we stop getting samples (no
  // battery is consumed) then the average value will 'migrate' from low
  // frequency durations (which will have data insufficiency due to no recent
  // samples) to higher ones because the duration captured will naturally get
  // longer and longer until it captures even the largest microDuration.
  // Since there are no datapoints the average will be very small and dwindle
  // to zero.  This is valid as we aren't requiring a minimum number of
  // datapoints to participate in the average but instead ensuring that we 
  // have enough data.  It also produces the right results as when we 
  // stop consuming battery then the larger durations will still show an
  // average value but the smaller ones will not.
  unsigned long int totalDurationMicros = getElapsed(maxPastTimeMicros, currentMicros);
  if (totalDurationMicros < microDuration * 0.80) { 
    myPrintf( "getApproximateAverage - returning insufficient data; "
              "microDuration: %lu, totalDurationMicros: %lu, difference: %lu\n", 
              microDuration, totalDurationMicros, 
              (microDuration - totalDurationMicros));
    return -1;
  }

  double secondsBetweenInterrupts = ((double)totalDurationMicros)/1000000.0;
  double result = (614.4 * interruptsAccumulated)/secondsBetweenInterrupts; 
  myPrintf( "getApproximateAverage - returning result;  currentMicros: %lu, "
                 "microDuration: %lu, result: %d\n", 
                 currentMicros, microDuration, (int)result);
  return result;
}

double 
DurationAdaptiveAverageTracker::getOverallAverage() {
  if (totalSeconds==0) { 
    return -1;
  }
  return (614.4 * totalInterrupts)/totalSeconds; 
}

// Return the number of elapsed microseconds that have occurred between time
// laterMicros and earlierMicros.  
unsigned long int 
DurationAdaptiveAverageTracker::getElapsed( unsigned long int earlierMicros, unsigned long int laterMicros) {
  unsigned long int result;
  // earlierMicros is assumed to the be the 'earlier' time but time may have wrapped
  // around.
  if (earlierMicros > laterMicros) { 
    // We have wrapped around since earlierMicros is supposed to be less.  
    // So it looks like this from a timeline perspective:
    //   0  |-----laterMicros---------earlierMicros-----| (ULONG_MAX)
    result = ( ULONG_MAX - earlierMicros + laterMicros );
  } else {
    result = laterMicros - earlierMicros;
  }
#ifdef DEBUG_ELAPSED_TIME
  myPrintf( "getElapsed - done;  earlierMicros: %lu, laterMicros: %lu, "
            "isEarlier: %d, result: %lu\n", 
    earlierMicros, laterMicros, (earlierMicros < laterMicros) ? 1 : 0, result);
#endif
  return result;
}


class GraphMilliAmpsAverageStats {
public:
  GraphMilliAmpsAverageStats(Adafruit_GFX &display, int topLeftX, int topLeftY, int width, int height);

  /**
   * call addDatasetValue whenever a value is available. 
   */
  void addDatasetValue(float deltaSecondsElapsed, float averagemAh);

  /**
   * call startRender a single time before the first call to render.
   */
  void startRender();

  /**
   * render will draw the average of all values passed in between this
   * call and the prior render.  If no data values are provided then
   * it assumed that the 0 is the latest value and render the graph
   * based on that.
   */
  void render();

private:

  uint8_t statsWidth();
  uint8_t statsHeight();
  uint8_t statsTopLeftX();
  uint8_t statsTopLeftY();

  Adafruit_GFX& display;
  uint16_t topLeftX, topLeftY;
  uint16_t width, height;

  float secondsToShow = 60;
  float pixelRatioForTime;
  float pixelRatioFormAh;

  int currentX;

  uint8_t borderX;
  uint8_t borderY;

  bool newDataPoint;
  float nextDeltaSecondsElapsed;
  float nextAveragemAh;

  float maxmAh;
};
  

GraphMilliAmpsAverageStats::GraphMilliAmpsAverageStats(Adafruit_GFX &d, 
    int topLeftX, int topLeftY, int width, int height) : display(d) {
  this->topLeftX = topLeftX;
  this->topLeftY = topLeftY;
  this->width = width;
  this->height = height;

  newDataPoint = false;
  maxmAh = 400;

  borderX = 4;
  borderY = 4;

  pixelRatioForTime = (double)(statsWidth()) / ((double)secondsToShow);
  pixelRatioFormAh = (double)(statsHeight()) / ((double)maxmAh);

  currentX = 0;
}

void
GraphMilliAmpsAverageStats::addDatasetValue(float deltaSecondsElapsed, float averagemAh) {
  nextDeltaSecondsElapsed = deltaSecondsElapsed;
  nextAveragemAh = averagemAh;
  newDataPoint = true;
}

uint8_t 
GraphMilliAmpsAverageStats::statsWidth() {
  return width - borderX;
}

uint8_t 
GraphMilliAmpsAverageStats::statsHeight() {
  return height - borderY;
}

uint8_t 
GraphMilliAmpsAverageStats::statsTopLeftX() { 
  return topLeftX + borderX;
}

uint8_t 
GraphMilliAmpsAverageStats::statsTopLeftY() { 
  return topLeftY;
}

void 
GraphMilliAmpsAverageStats::startRender() {
  // Clear the area
  display.fillRect(topLeftX, topLeftY, width, height, BLACK);
  display.drawFastHLine(topLeftX, topLeftY + height - (borderY-1), width, RED);
  display.drawFastVLine(topLeftX+(borderX-1), topLeftY, height, RED);
}

void
GraphMilliAmpsAverageStats::render() { 
  if (newDataPoint) { 
    int w = nextDeltaSecondsElapsed * pixelRatioForTime;
    int h = nextAveragemAh * pixelRatioFormAh;

    if (h > statsHeight()) { 
      h = statsHeight();
    }
    if (w > statsWidth()) {
      w = statsWidth();
    }
    
    // Clear the space at this position as well as beyond. 
    display.fillRect(statsTopLeftX() + currentX, statsTopLeftY(), 25, statsHeight(), BLACK);

    // Draw the actual datapoint
    display.fillRect(statsTopLeftX() + currentX, statsTopLeftY() + (statsHeight()-h), w, h, BLUE);

    currentX += w + 1;
    if (currentX + 3 >= statsWidth()) { 
      currentX = 0;
    } else {
      // Draw the indicator showing where we are.
      for (int y=0; y<statsHeight(); y+=5) { 
        uint8_t lineHeight = 3;
        if (y + lineHeight > statsHeight()) { 
          lineHeight = statsHeight() - y;
        }
        display.drawFastVLine(statsTopLeftX() + currentX, statsTopLeftY() + y, lineHeight, YELLOW);
      }
    }
    newDataPoint = false;
  }
}


class SmartStatDisplay {
public:
  SmartStatDisplay(Adafruit_GFX &display, int topLeftX, int topLeftY, int width, int height);

  /*
   * render the value within the slot that has been defined for this element.
   * If the value has not changed then this won't do any work.
   */
  void render(float value);

private:
  Adafruit_GFX& display;
  uint16_t topLeftX, topLeftY;
  uint16_t width, height;

  bool firstValue;
  float priorValue;
};

SmartStatDisplay::SmartStatDisplay(Adafruit_GFX &d, 
    int topLeftX, int topLeftY, int width, int height) : display(d) {
  this->topLeftX = topLeftX;
  this->topLeftY = topLeftY;
  this->width = width;
  this->height = height;

  firstValue = true;
  priorValue = 0;
}

void
SmartStatDisplay::render(float value) { 
  if (firstValue == true || priorValue != value ) {

    // Clear the space for this element
    display.fillRect(topLeftX, topLeftY, width, height, BLACK);

    display.setCursor(topLeftX, topLeftY);
    display.print(value);

    priorValue = value;
    firstValue = false;
  }
}


// Track two micro times.  The first, startZeroInterruptMicros, is set during the main
// routine, outside of an interrupt, when the numberOfInterrupts is set to zero.  
// The combination of startZeroInterruptMicros and numberOfInterrupts tells us how much
// current has been used during a period of time.  The mostRecentInterruptMicros is set
// by the ISR and allows us to be precise, independent of the screen update periodicity, 
// of how much current has been consumed.  
unsigned long int startZeroInterruptMicros;
volatile unsigned long int mostRecentInterruptMicros;
volatile uint8_t numberOfInterrupts;

// Every time the interrupt fires we know that 0.17607 milli amps, or 170.6 micro amps, 
// have been consumed from the battery.  
double mAhQuanta = 0.17067759; 

unsigned long int startTime;

unsigned int totalNumberOfInterrupts;

double averagemAh;
double totalConsumedmAh;

DurationAdaptiveAverageTracker t(200);
GraphMilliAmpsAverageStats *g;
SmartStatDisplay *statmAh;
SmartStatDisplay *statTotalmAh;
SmartStatDisplay *statOverallAvgmAh;
SmartStatDisplay *stat20SecAvgmAh;
SmartStatDisplay *stat60SecAvgmAh;
SmartStatDisplay *stat120SecAvgmAh;

void interruptForCoulomb() {
  mostRecentInterruptMicros = micros();
  numberOfInterrupts++;
}

void setup() {
  Serial.begin(9600);

  // Initialize the OLED
  tft.begin();
  tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);

  // Initialize the interrupt pin used to receive pulses from the
  // Sparkfun coulomb counter.
  pinMode(COULOMB_INTERRUPT_PIN,INPUT); 
  attachInterrupt(digitalPinToInterrupt(COULOMB_INTERRUPT_PIN),interruptForCoulomb,FALLING);

  mostRecentInterruptMicros = 0;
  startZeroInterruptMicros = micros();
  numberOfInterrupts = 0;

  averagemAh = 0;
  totalConsumedmAh = 0;

  totalNumberOfInterrupts = 0;

  startTime = millis();

  tft.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
  g = new GraphMilliAmpsAverageStats( tft, 5, 2, SCREEN_WIDTH - (5*2), 50 );
  g->startRender();

  statmAh = new SmartStatDisplay(tft, 40, 53, 42, 8);
  statTotalmAh = new SmartStatDisplay(tft, 40, 63, 42, 8);
  statOverallAvgmAh = new SmartStatDisplay(tft, 71, 87, 42, 8);
  stat20SecAvgmAh = new SmartStatDisplay(tft, 71, 97, 42, 8);
  stat60SecAvgmAh = new SmartStatDisplay(tft, 71, 107, 42, 8);
  stat120SecAvgmAh = new SmartStatDisplay(tft, 71, 117, 42, 8);

  tft.setCursor(12,53);
  tft.setTextColor(WHITE);
  tft.print("mAh:");

  tft.setCursor(0,63);
  tft.print("Total:");

  tft.fillRect(2, 73, SCREEN_WIDTH-4, 1, YELLOW);
  tft.setCursor(30,75);
  tft.setTextColor(YELLOW);
  tft.print("Average mAhs");

  tft.setCursor(19,87);
  tft.setTextColor(WHITE);
  tft.print("Overall: ");

  tft.setCursor(19,97);
  tft.print("20 secs: ");

  tft.setCursor(19,107);
  tft.print("60 secs: ");

  tft.setCursor(13,117);
  tft.print("120 secs: ");

  Serial.println("setup complete!");
}

double secondsBetweenInterrupts = 0;

void loop() {
  if (numberOfInterrupts) {
    t.addData(micros(), numberOfInterrupts);

    secondsBetweenInterrupts = (mostRecentInterruptMicros-startZeroInterruptMicros)/1000000.0;
    averagemAh = (614.4 * numberOfInterrupts)/secondsBetweenInterrupts; 
    totalConsumedmAh += (mAhQuanta * numberOfInterrupts);
 
    totalNumberOfInterrupts += numberOfInterrupts;
    startZeroInterruptMicros = micros();
    numberOfInterrupts = 0;

    g->addDatasetValue(secondsBetweenInterrupts, averagemAh);
  }
  unsigned long int currentMicros = micros();

  statmAh->render(averagemAh);
  statTotalmAh->render(totalConsumedmAh);
  statOverallAvgmAh->render(t.getOverallAverage());
  stat20SecAvgmAh->render(t.getApproximateAverage(currentMicros, 20 * 1000 * 1000));
  stat60SecAvgmAh->render(t.getApproximateAverage(currentMicros, 60 * 1000 * 1000));
  stat120SecAvgmAh->render(t.getApproximateAverage(currentMicros, 120 * 1000 * 1000));

  g->render();

  delay(2000);
}



