
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
#define LED_PIN    13

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
// the Serial rather than Serial.
#define Serial SerialUSB

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

class CurrentDatapoints {
public:
  CurrentDatapoints(int numberToTrack);

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


CurrentDatapoints::CurrentDatapoints(int numberToTrack) { 
  allocatedDatapoints = numberToTrack;
  numberDatapoints = 0;
  nextIndex = 0;
  d = new Datapoint[allocatedDatapoints];
  memset( d, 0, sizeof(Datapoint)*allocatedDatapoints);
  lastMicrosSeen = 0;
  totalSeconds = 0;
}


void CurrentDatapoints::addData(unsigned long int timeMicros, uint8_t numberOfInterrupts) {
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
CurrentDatapoints::getApproximateAverage( unsigned long int currentMicros, unsigned long int microDuration ) {
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
  if (totalDurationMicros < microDuration * 0.85) { 
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
CurrentDatapoints::getOverallAverage() {
  if (totalSeconds==0) { 
    return -1;
  }
  return (614.4 * totalInterrupts)/totalSeconds; 
}

// Return the number of elapsed microseconds that have occurred between time
// laterMicros and earlierMicros.  
unsigned long int 
CurrentDatapoints::getElapsed( unsigned long int earlierMicros, unsigned long int laterMicros) {
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

CurrentDatapoints c(200);

void interruptForCoulomb() {
  mostRecentInterruptMicros = micros();
  numberOfInterrupts++;
}

void setup() {
  Serial.begin(9600);

  // Illuminate status LED_PIN.
  pinMode(LED_PIN, INPUT_PULLUP); 
     
  // Initialize the OLED
  tft.begin();
  tft.fillRect(0, 0, 128, 128, BLACK);

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
  Serial.println("setup complete!");
}

double secondsBetweenInterrupts = 0;

void loop() {
  if (numberOfInterrupts) {
    c.addData(micros(), numberOfInterrupts);

    secondsBetweenInterrupts = (mostRecentInterruptMicros-startZeroInterruptMicros)/1000000.0;
    averagemAh = (614.4 * numberOfInterrupts)/secondsBetweenInterrupts; 
    totalConsumedmAh += (mAhQuanta * numberOfInterrupts);
 
    totalNumberOfInterrupts += numberOfInterrupts;
    startZeroInterruptMicros = micros();
    numberOfInterrupts = 0;
  }

  unsigned long int currentTime = millis();

  tft.fillRect(0, 0, 128, 128, BLACK);
  tft.setCursor(0,0);
  tft.setTextColor(WHITE);
  tft.print( "Seconds: ");
  tft.println( (int) ((currentTime - startTime) / 1000));
  tft.println();

  tft.print("Interrupts: ");
  tft.println(totalNumberOfInterrupts);
  tft.print("Avg mAh: ");
  tft.println(averagemAh);
  tft.print("  (");
  tft.print(secondsBetweenInterrupts);
  tft.println(" secs)");
  tft.print("Total mAh: ");
  tft.println(totalConsumedmAh);
  tft.println("---");
  tft.print("C-Overall: ");
  tft.println(c.getOverallAverage());
  unsigned long int currentMicros = micros();
  tft.print("C-20s: ");
  tft.println(c.getApproximateAverage(currentMicros, 20 * 1000 * 1000));
  tft.print("C-60s: ");
  tft.println(c.getApproximateAverage(currentMicros, 60 * 1000 * 1000));
  tft.print("C-120s: ");
  tft.println(c.getApproximateAverage(currentMicros, 120 * 1000 * 1000));
  tft.print("C-360s: ");
  tft.println(c.getApproximateAverage(currentMicros, 360 * 1000 * 1000));
  delay(2000);
}



