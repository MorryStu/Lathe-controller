/*
V3 - inludes Quad Encoder and FreqMeasureMulti because I fried pin 22

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
Stepper Pin14 = Stepper pulse
        pin15 = Stepper Dir    
DISP1 Pin16 = DISP1Clk - TM1637 (4 x 7seg led) display
      Pin17 = DISP1Dat
DISP2 Pin18 = DISP2Clk - TM1637 (4 x 7seg led) display
      Pin19 = DISP2Dat
ENC Pin20 = ENCClk - KY040 rotary encoder
    PIN21 = ENCDat
RPM Pin23 - Hardware pin using freqmeasuremulti
SPINDLE Pin31 = SPINDLE_A - Hardware pins 1000pps quadrature encoder
        Pin33 = SPINDLE_b
        */
#include <RotaryEncoder.h>  //Rotary_Encoder_KY-040_Fixed-main/RotaryEncoder.h https://github.com/ownprox/Rotary_Encoder_KY-040_Fixed.....NOTE. this installs as RotaryEncoder.h, so make sure to remove any existing library
#include <Bounce2.h>
#include <SPI.h>
#include <Wire.h>
#include <TM1637Display.h>
#include <QuadEncoder.h>
//#include <FreqMeasure.h>
#include <teensystep4.h>
#include <FreqMeasureMulti.h>
#include <FreqMeasureMultiIMXRT.h>

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
#define stepPin 14
#define directionPin 15
#define CLK1 16  //Display 1 Clk
#define DIO1 17  // Display 1 data
#define CLK2 18  // Display 2 Clk
#define DIO2 19  // Display 2 data

TM1637Display display1(CLK1, DIO1);
TM1637Display display2(CLK2, DIO2);

Bounce MMdebouncer = Bounce();
Bounce TPIdebouncer = Bounce();
Bounce FEEDdebouncer = Bounce();
Bounce STOPdebouncer = Bounce();
Bounce FWDdebouncer = Bounce();
Bounce REVdebouncer = Bounce();


using namespace TS4;        //  Required setup for Teensystep4
Stepper LeadScrew(14, 15);  //


QuadEncoder spindleEnc(1, 31, 33, 1);  // Encoder on channel 1 of 4 available // Channel (1), Phase A (pin0), PhaseB(pin1), Pullups Req(1=intPullup). FIXED HARWARE PINS
FreqMeasureMulti Freq1; // pin 23

int ledState = LOW;  // SET A VARIABLE TO STORE THE LED STATE
int Counter = 0;
int LastCount = 0;

int Countermm = Counter+100;
int Countertpi = Counter;
int Counterfeed = Counter;
float frequency = 0;
float RPM = 0;
double sum = 0;
int count = 0;
long newSpindle = 0;
long oldPosition = -999;  // Spindle encoder
long long leadscrewPos = 0000000.0000000;

float Pitchmm;  // used for metric leadscrew calculation and stepper config
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
  delay(10);
  pinMode(23, INPUT_PULLDOWN);
   Freq1.begin(23);
  Serial.println("T.I.S.M v3");  // Death to ART tour, featuring Great Trucking Songs of The Renaissance

  display1.setBrightness(0x0f);
  display2.setBrightness(0x0f);
  delay(500);

  display1.clear();
  display2.clear();
  display1.showNumberDecEx(8080, 0b01000000, false, 4, 4);  //Displays 8888
  display2.showNumberDec(1234, false, 4, 0);                // shows 1234

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
  newSpindle = 0;
  newSpindle = spindleEnc.read();
   if (newSpindle != oldPosition) {
  //Serial.print("Spindle = ");
  //Serial.print(newSpindle);
  //Serial.println();
  oldPosition = newSpindle;
   }
 if (Freq1.available()) {
    sum = sum + Freq1.read();
    count = count + 1;
    if (count > 5) { // average several reading together
      frequency = Freq1.countToFrequency(sum / count);
      RPM = frequency / 0.016;
      Serial.print("rpm");
      Serial.println(RPM);
      display1.clear();
      display1.showNumberDec(RPM, false, 4, 4);
      sum = 0;
      count = 0;
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

void readButton() {

  MMdebouncer.update();      // Update the Bounce instance
  if (MMdebouncer.fell()) {  // Call code if button transitions from HIGH to LOW
    ledState = !ledState;    // Toggle LED state
    digitalWrite(MMLed_PIN, ledState);
    mode = MM;

  } else

    TPIdebouncer.update();
  if (TPIdebouncer.fell()) {
    ledState = !ledState;
    digitalWrite(TPILed_PIN, ledState);
    mode = TPI;

  } else
    FEEDdebouncer.update();
  if (FEEDdebouncer.fell()) {
    ledState = !ledState;
    digitalWrite(FEEDLed_PIN, ledState);
    mode = FEED;
  }
}
void mm() {
  //Serial.println("void mm");
  digitalWrite(MMLed_PIN, HIGH);
  digitalWrite(TPILed_PIN, LOW);
  digitalWrite(FEEDLed_PIN, LOW);
  if (LastCount != Countermm) {
    //Serial.println(Counter);
    Pitchmm = Countermm / 100.00;  //Metric threads have dec point values. i.e 1.25mm
    display2.clear();
    display2.showNumberDecEx(Counter, 0b10000000, true, 3, 1);  // Use Counter value for deciml point placement on LED disp
    //Serial.println(Pitchmm);
  }
}

void tpi() {
  digitalWrite(TPILed_PIN, HIGH);
  digitalWrite(MMLed_PIN, LOW);
  digitalWrite(FEEDLed_PIN, LOW);
  //Serial.println("void tpi");
  if (LastCount != Countertpi) {
    Pitchtpi = Countertpi - 100;  // 100;  // value in TPI
    //Serial.println(Pitchtpi);
    display2.clear();
    display2.showNumberDec(Pitchtpi, false, 3, 0);
  }
}

void feed() {
  digitalWrite(FEEDLed_PIN, HIGH);
  digitalWrite(MMLed_PIN, LOW);
  digitalWrite(TPILed_PIN, LOW);
  //Serial.println("feedmm");
  if (LastCount != Counterfeed) {
    //Serial.println(Counter);
    //Pitchtpi = Counter;  // value for feed rate
    display2.clear();
    display2.showNumberDec(Pitchtpi, false, 3, 0);
    FWDdebouncer.update();      // Update the Bounce instance
    if (FWDdebouncer.fell()) {  // Call code if button transitions from HIGH to LOW
      ledState = !ledState;     // Toggle LED state
      digitalWrite(FWDLed_PIN, ledState);
      LeadScrew.rotateAsync(Counterfeed * 10);
      if (FWDdebouncer.fell()) {  // Call code if button transitions from HIGH to LOW
        ledState = !ledState;     // Toggle LED state
        digitalWrite(FWDLed_PIN, ledState);
        LeadScrew.stop();
      }
    }
  }
}