//  ============================================================
//
//  Program: ArdCore Sequencer
//
//  Description: A basic 8 step sequencer, where the input
//               is done using the two ArdCore knobs and/or CV inputs.
//               Originally created by 20 Objects and modified by the Tuesday Night Machines (TTNM).
//
//  I/O Usage:
//    Knob 1: Pitch to be recorded at the current step (works as offset when used with Analog In 1)
//    Knob 2: Current step selection (works as offset when used with Analog In 2)
//    Analog In 1: CV control over the current step's pitch
//    Analog In 2: CV control over step selection
//    Digital Out 1: Trigger on step output
//    Digital Out 2: Random chance trigger between 25% - 75% (depending on A1 knob position)
//    Clock In: External clock input
//    Analog Out: 8-bit output
//
//  Input Expander: unused
//  Output Expander: 8 bits of output exposed
//
//  Created:  10 Dec 2010
//  Modified: 13 Feb 2011 - rework to use knobs only
//            12 Mar 2011 - Added pause function for step selection
//                        - Added digital out firing on current step
//                        - Fixed gate output turnoff
//            17 Apr 2012  ddg Updated for Arduino 1.0
//            18 Apr 2012  ddg Changed dacOutput routine to Alba version
//            04 Jan 2018  analog CV input and random trigger functionality added by The Tuesday Night Machines (TTNM)
//
//  ============================================================
//
//  License:
//
//  This software is licensed under the Creative Commons
//  "Attribution-NonCommercial license. This license allows you
//  to tweak and build upon the code for non-commercial purposes,
//  without the requirement to license derivative works on the
//  same terms. If you wish to use this (or derived) work for
//  commercial work, please contact 20 Objects LLC at our website
//  (www.20objects.com).
//
//  For more information on the Creative Commons CC BY-NC license,
//  visit http://creativecommons.org/licenses/
//
//  ================= start of global section ==================

//  constants related to the Arduino Nano pin use
const int clkIn = 2;           // the digital (clock) input
const int digPin[2] = {3, 4};  // the digital output pins
const int pinOffset = 5;       // the first DAC pin (from 5-12)
const int trigTime = 25;       // the standard trigger time

//  variables for interrupt handling of the clock input
volatile int clkState = LOW;

//  variables used to control the current DIO output states
int digState[2] = {LOW, LOW};  // start with both set low
unsigned long digTime[2] = {0, 0};      // the times of the last HIGH

//  recording and playback variables
int seqValue[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int maxSeqValue = 8;
int currSeqPlay = 0;

//  limit changes to the sequencer to require a 100ms pause
int lastPos = -1;
unsigned long lastPosMillis = 0;

//  ==================== start of setup() ======================

void setup() {
  // set up the digital (clock) input
  pinMode(clkIn, INPUT);
  
  // set up the digital outputs
  for (int i=0; i<2; i++) {
    pinMode(digPin[i], OUTPUT);
    digitalWrite(digPin[i], LOW);
  }
  
  // set up the 8-bit DAC output pins
  for (int i=0; i<8; i++) {
    pinMode(pinOffset+i, OUTPUT);
    digitalWrite(pinOffset+i, LOW);
  }
  
  attachInterrupt(0, isr, RISING);
}

//  ==================== start of loop() =======================

void loop()
{
  int tempPos = analogRead(1) + analogRead(3)  >> 7; // TTNM: added A3 analog CV input

  // on clock tick, move forward and play.
  if (clkState == HIGH) {
    clkState = LOW;
    
    currSeqPlay = (++currSeqPlay % maxSeqValue);
    dacOutput(seqValue[currSeqPlay]);
    
    digState[0] = HIGH;
    digTime[0] = millis();
    digitalWrite(digPin[0], HIGH);

    // TTNM: D1 outputs random trigger at about 25% - 75% chance, depending on A1 knob position (CCW ca. 25%, CW ca. 75%)
    if (random(0,100) < 25 + (analogRead(1)/15)) {
          digState[1] = HIGH;
          digTime[1] = millis();
          digitalWrite(digPin[1], HIGH);
    }


  }

  // record the current knob position values
  if (tempPos != lastPos) {
    lastPos = tempPos;
    lastPosMillis = millis();
  }

  // TTNM: if A3 CV input is not used (A3 pot turned to 0) then write note changes after 250ms pause, so that manual step switching doesn't overwrite all steps immediately
  int NewPauseTime = 250;

  // TTNM: if A3 CV input is used (A3 pot turned up) then wrute note change immediately
  if (analogRead(3) > 0) {
    NewPauseTime = 0;
  }

  if(millis() - lastPosMillis > NewPauseTime) {
    int tempVal = quantNote(analogRead(0) + analogRead(2)); // TTNM: added A2 analog CV input
    seqValue[lastPos] = tempVal;
  }
  
  // turn off the gate when required
  for (int i=0; i<2; i++) {
    if ((millis() - digTime[i] > trigTime) && (digState[i] == HIGH)) {
      digState[i] = LOW;
      digitalWrite(digPin[i], LOW);
    }
  }
}

//  =================== convenience routines ===================

//  isr() - quickly handle interrupts from the clock input
//  ------------------------------------------------------
void isr()
{
  clkState = HIGH;
}

//  dacOutput(byte) - deal with the DAC output
//  -----------------------------------------
void dacOutput(byte v)
{
  PORTB = (PORTB & B11100000) | (v >> 3);
  PORTD = (PORTD & B00011111) | ((v & B00000111) << 5);
}

//  quantNote(int) - drop an incoming value to a note value
//  -------------------------------------------------------
int quantNote(int v)
{
  // feed this routine the input from one of the analog inputs
  // and it will return the value in a 0-64 range.
  return (v >> 4) << 2;
}

//  ===================== end of program =======================
