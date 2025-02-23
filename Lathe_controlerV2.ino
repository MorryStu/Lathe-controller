/*
MM Pin0 = MMButt
   Pin1 = MMLed
TPI Pin2 = TPIButt
    Pin3 = TPILed
Feed Pin4 = FeedButt
     Pin5 = FeedLED
STOP Pin6 = STOPButt
     Pin7 = STOPLed
FWD Pin8 = FWDButt
    Pin9 = FWDLED
REV Pin10 = REVButt
    Pin11 = REVLED
DISP1 Pin16 = DISP1Clk
      Pin17 = DISP1Dat
DISP2 Pin18 = DISP2Clk
      Pin19 = DISP2Dat
ENC Pin20 = ENCClk
    PIN21 = ENCDat
SPINDLE Pin22 = SPINDLE_A
        Pin23 = SPINDLE_b
        */
#include <RotaryEncoder.h>  //Rotary_Encoder_KY-040_Fixed-main/RotaryEncoder.h https://github.com/ownprox/Rotary_Encoder_KY-040_Fixed.....NOTE. this installs as RotaryEncoder.h, so make sure to remove any existing library
#include <Bounce2.h>
#include <SPI.h>
#include <Wire.h>
#include <TM1637Display.h>
//#include <QuadEncoder.h>
#include <Encoder.h>

#define MMButt_PIN 0  // WE WILL attach() THE BUTTON TO THE FOLLOWING PIN IN setup()
#define MMLed_PIN 1   // DEFINE THE PIN FOR THE LED :
#define TPIButt_PIN 2
#define TPILed_PIN 3
#define FEEDButt_PIN 4
#define FEEDLed_PIN 5
#define STOPButt_PIN 6
#define STOPLed_PIN 7
#define FWDButt_PIN 8
#define FWDLed_PIN 9
#define REVButt_PIN 10
#define REVLed_PIN 11
#define CLK1 16  //Display 1 Clk
#define DIO1 17  // Display 1 data
#define CLK2 18  // Display 2 Clk
#define DIO2 19  // Display 2 data

TM1637Display display1(CLK1, DIO1);
TM1637Display display2(CLK2, DIO2);

Encoder SpindleEnc(22, 23);

Bounce MMdebouncer = Bounce();
Bounce TPIdebouncer = Bounce();
Bounce FEEDdebouncer = Bounce();
Bounce STOPdebouncer = Bounce();
Bounce FWDdebouncer = Bounce();
Bounce REVdebouncer = Bounce();

int ledState = LOW;  // SET A VARIABLE TO STORE THE LED STATE
int Counter = 100, LastCount = 0;
float Countermm = Counter;
float Countertpi = 0;
float Counterfeed = 0;

long oldPosition = -999;  // for spindle
float Pitchmm;            // used for metric leadscrew calculation and stepper config
float Pitchtpi;

enum MODE  // available modes
{
  MM,
  TPI,
  FEED,
};
MODE mode = MM;

void RotaryChanged();
RotaryEncoder Rotary(&RotaryChanged, 20, 21, 28);  // Pins 20 (DT), 21 (CLK), 28 (SW) - 28 is just a null number as the (SW) is not used

void RotaryChanged() {
  const unsigned int state = Rotary.GetState();
  if (state & DIR_CW)
    Counter++;
  if (state & DIR_CCW)
    Counter--;
}


void setup() {
  Serial.begin(9600);
  Serial.println("T.I.S.M");  // Death to ART tour, featuring Great Trucking Songs of The Renaissance

  display1.setBrightness(0x0f);
  display2.setBrightness(0x0f);
  delay(500);

  display1.clear();
  display2.clear();
  display1.showNumberDecEx(8888, 0b01000000, false, 4, 4);  //Displays 10.11
  display2.showNumberDec(1234, false, 4, 0);                // shows 10 on far left

  MMdebouncer.attach(MMButt_PIN, INPUT_PULLUP);  // Attach the debouncer to a pin with INPUT_PULLUP mode
  MMdebouncer.interval(25);                      // Use a debounce interval of 25 milliseconds
  pinMode(MMLed_PIN, OUTPUT);                    // Setup the LED
  digitalWrite(MMLed_PIN, ledState);

  TPIdebouncer.attach(TPIButt_PIN, INPUT_PULLUP);
  TPIdebouncer.interval(25);
  pinMode(TPILed_PIN, OUTPUT);
  digitalWrite(TPILed_PIN, ledState);

  FEEDdebouncer.attach(FEEDButt_PIN, INPUT_PULLUP);
  FEEDdebouncer.interval(25);
  pinMode(FEEDLed_PIN, OUTPUT);
  digitalWrite(FEEDLed_PIN, ledState);

  STOPdebouncer.attach(STOPButt_PIN, INPUT_PULLUP);
  STOPdebouncer.interval(25);
  pinMode(STOPLed_PIN, OUTPUT);
  digitalWrite(STOPLed_PIN, ledState);

  FWDdebouncer.attach(FWDButt_PIN, INPUT_PULLUP);
  FWDdebouncer.interval(25);
  pinMode(FWDLed_PIN, OUTPUT);
  digitalWrite(FWDLed_PIN, ledState);

  REVdebouncer.attach(REVButt_PIN, INPUT_PULLUP);
  REVdebouncer.interval(25);
  pinMode(REVLed_PIN, OUTPUT);
  digitalWrite(REVLed_PIN, ledState);
}
void loop() {
  {
    long RPM;
    long newPosition = SpindleEnc.read();
    if (newPosition != oldPosition) {
      oldPosition = newPosition;
      //Serial.println(newPosition);
RPM = (newPosition/?); // some code that involves millis.  STRICTLY NO  Delay(); EVER...anywhere
display1.clear();
display1.showNumberDec(RPM, false, 2, 1);
    }
  }
  readButton();  // read button and update mode
  switch (mode) {
    case MM:
      mm();
      break;
    case TPI:
      tpi();
      break;
    case FEED:
      feed();
      break;
  }
}



// read button and update mode

void readButton() {

  MMdebouncer.update();      // Update the Bounce instance
  if (MMdebouncer.fell()) {  // Call code if button transitions from HIGH to LOW
    ledState = !ledState;    // Toggle LED state
    digitalWrite(MMLed_PIN, ledState);
    switch (mode)
    case MM:
    default:
      break;
  } else

    TPIdebouncer.update();
  if (TPIdebouncer.fell()) {
    ledState = !ledState;
    digitalWrite(TPILed_PIN, ledState);
    switch (mode)
    case TPI:
    default:
      break;
  } else
    FEEDdebouncer.update();
  if (FEEDdebouncer.fell()) {
    ledState = !ledState;
    digitalWrite(FEEDLed_PIN, ledState);
    switch (mode)
    case FEED:
    default:
      break;
  }
}
void mm() {
  Serial.println("void mm");
  if (LastCount != Counter) {
    //Serial.println(Counter);
    Pitchmm = Counter / 100.00;  //Metric threads have dec point values. i.e 1.25mm
    display2.clear();
    display2.showNumberDecEx(Counter, 0b10000000, true, 3, 1);  // Use Counter value for deciml point placement on LED disp
    Serial.println(Pitchmm);
  }
}

void tpi() {
  Serial.println("void tpi");
  if (LastCount != Counter) {
    Serial.println(Counter);
    Pitchtpi = Counter;  // value in TPI
    display2.clear();
    display2.showNumberDec(Pitchtpi, false, 2, 1);
  }
}

void feed() {
  Serial.println("feed tpi");
  if (LastCount != Counter) {
    Serial.println(Counter);
    Pitchtpi = Counter;  // value in TPI
    display2.clear();
    display2.showNumberDec(Pitchtpi, false, 2, 1);
  }
}