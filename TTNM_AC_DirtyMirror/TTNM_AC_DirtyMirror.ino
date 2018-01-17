//  ============================================================
//
//  Program: Dirty Mirror by The Tuesday Night Machines #TTNM
//
//  Description: A wavefolder and wave-combiner/mixer/multiplier for CV and audio, with a clock divider output.
//
//  Demo video: https://www.youtube.com/watch?v=weYYc37__aU&list=PLa9Em_H8Xs_b2zcSwXT3ZHS4q_2oWKTOA&index=13
//
//  I/O Usage:
//    Knob A0: Threshold above which Wave 1 will be output, below will be the inverted Wave 2
//    Knob A1: Clock divier value (higher = higher division, i.e. slower clock)
//
//    Knob A2: Attenuator for Wave 1 input
//    Analog In 2: Input for Wave 1 (unipolar, scaled to 0-5V, input above 5V will fold over)
//    Knob A3: Attenuator for Wave 2 input
//    Analog In 3: Input for Wave 2 (unipolar, scaled to 0-5V, input above 5V will fold over)
//
//    Digital Out D0: Trigger on Wave 1 input above Threshold, divided Clock Divider
//    Digital Out D1: Trigger on Wave 2 input above Threshold, divided Clock Divider
//
//    Clock In: Cycle through modes: combiner (A2 above threshold, inverted A3 below), mixer (A2-A3), multiplier (A2*A3) and silence (patch D0 to Clock In for rhythmic fun)
//    Analog Out: Waveform output (unipolar, 0-5V)
//
//
//  Input Expander: unused
//  Output Expander: unused
//
//  Created:  07 Jan 2018 by TTNM (using 20 Objects' Ardcore template)
//  Modified: 16 Jan 2018 by TTNM (clean up, added clock input to cycle through modes)
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
//  commercial work, please contact The Tuesday Night Machines (TTNM) at our website
//  (http://nightmachines.tv).
//
//  For more information on the Creative Commons CC BY-NC license,
//  visit http://creativecommons.org/licenses/
//
//  ================= start of global section ==================

//  constants related to the Arduino Nano pin use
const int clkIn = 2;           // the digital (clock) input
const int digPin[2] = {3, 4};  // the digital output pins
const int pinOffset = 5;       // the first DAC pin (from 5-12)
const int trigTime = 25;       // 25 ms trigger timing

const int oct[6] = {0, 48, 96, 144, 192, 240}; // the six octave values

//  variables for interrupt handling of the clock input
volatile int clkState = LOW;
int clkDivide = 0;

//  variables used to control the current DIO output states
int digState[2] = {LOW, LOW};        // start with both set low
unsigned long digMilli[2] = {0, 0};  // a place to store millis()

int dacPreOut = 0;

int counter1 = 0;
bool state1 = false;
bool lastState1 = false;

int counter2 = 0;
bool state2 = false;
bool lastState2 = false;

int waveMode = 0;
float multVal = 0.0;

//  ==================== start of setup() ======================

//  This setup routine should be used in any ArdCore sketch that
//  you choose to write; it sets up the pin usage, and sets up
//  initial state. Failure to properly set up the pin usage may
//  lead to damaging the Arduino hardware, or at the very least
//  cause your program to be unstable.

void setup() 
{
 
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
  
  // set up an interrupt handler for the clock in. If you
  // aren't going to use clock input, you should probably
  // comment out this call.
  // Note: Interrupt 0 is for pin 2 (clkIn)
  attachInterrupt(0, isr, RISING);

  
}


void loop() 
{

  // WAVEFORMS
  if (clkState == HIGH){
    clkState = LOW;
    waveMode++;
    if (waveMode > 3){
      waveMode = 0;
    }
  }

  if (waveMode == 0){                         // dirty mirror combiner
  
    if (analogRead(2) > analogRead(0)/2) {     // if A2 above threshold (A0) the output A2
        dacPreOut = analogRead(2)/2;                
    }else {             
        dacPreOut = ((analogRead(3)*-1) + (analogRead(0)/2))/2;    // if A2 below threshold the output A3*-1
    }
    
  }else if (waveMode == 1){                   // wave subtract mixer
    dacPreOut = analogRead(2) - analogRead(3); 
  }else if (waveMode == 2){                   // wave multiplier
    multVal = analogRead(3)/1023.0;
    dacPreOut = analogRead(2) * multVal; 
  }else if (waveMode == 3){                   // silence
    dacPreOut = 0;
  }

  //convert to 8bit and fold/clip anything above/below
  if (dacPreOut > 255){
    dacPreOut = (255-(dacPreOut-255));
  }
  if(dacPreOut < 0){
    dacPreOut = dacPreOut*-1;
  }
  if (dacPreOut > 255){
    dacPreOut = (255-(dacPreOut-255));
  }
  
  dacOutput(dacPreOut);
  
  



   // OUTPUT D0 Triggers

   lastState1 = state1;
   
   if (analogRead(2) > analogRead(0)) {
      state1 = true;           
   }else {              
      state1 = false;     
   }
   
   if (lastState1 != state1) {
      counter1++;
   }
   if (analogRead(1)/32 == 0) {
      counter1 = 0;
   }
   if (counter1/(((analogRead(1))/32)/2) == 1) {
      digitalWrite(digPin[0], HIGH);
   }
   if (counter1/(((analogRead(1))/32)) == 1) {
      digitalWrite(digPin[0], LOW);
      counter1 = 0;
   }
   
     

   // OUTPUT D1 Triggers
   
   lastState2 = state2;
    
   if (analogRead(3) > analogRead(0)) {
      state2 = true;
   }else{
       state2 = false;
   }

   if (lastState2 != state2) {
      counter2++;
   }
   if (analogRead(1)/32 == 0) {
      counter2 = 0;
   }
   if (counter2/((analogRead(1)/16)/2) == 1) {
      digitalWrite(digPin[1], HIGH);
   }
   if (counter2/((analogRead(1)/16)) == 1) {
      digitalWrite(digPin[1], LOW);
      counter2 = 0;
   }

}


//  =================== convenience routines ===================

//  These routines are some things you will need to use for
//  various functions of the hardware. Explanations are provided
//  to help you know when to use them.

//  isr() - quickly handle interrupts from the clock input
//  ------------------------------------------------------
void isr()
{
  // Note: you don't want to spend a lot of time here, because
  // it interrupts the activity of the rest of your program.
  // In most cases, you just want to set a variable and get
  // out.
  clkState = HIGH;
}

//  dacOutput(long) - deal with the DAC output
//  ------------------------------------------
void dacOutput(byte v)
{
  /*
  // feed this routine a value between 0 and 255 and teh DAC
  // output will send it out.
  int tmpVal = v;
  for (int i=0; i<8; i++) {
    digitalWrite(pinOffset + i, tmpVal & 1);
    tmpVal = tmpVal >> 1;
  }
  */
  
  // replacement routine as suggested by Alphonso Alba
  // this code accomplishes the same thing as the original
  // code from above, but is approx 4x faster
  PORTB = (PORTB & B11100000) | (v >> 3);
  PORTD = (PORTD & B00011111) | ((v & B00000111) << 5);
}

//  deJitter(int, int) - smooth jitter input
//  ----------------------------------------
int deJitter(int v, int test)
{
  // this routine just make sure we have a significant value
  // change before we bother implementing it. This is useful
  // for cleaning up jittery analog inputs.
  if (abs(v - test) > 8) {
    return v;
  }
  return test;
}

//  quantNote(int) - drop an incoming value to a note value
//  -------------------------------------------------------
int quantNote(int v)
{
  // feed this routine the input from one of the analog inputs
  // and it will return the value in a 0-64 range, which is
  // roughly the equivalent of a 0-5V range.
  return (v >> 4) << 2;
}

//  ===================== end of program =======================
