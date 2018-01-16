//  ============================================================
//
//  Program: Bit Beats by The Tuesday Night Machines #TTNM
//
//  Description: An 8-step trigger sequencer and a monophonic, lo-fi chiptune groovebox with three drum sounds and three synth waveforms.
//
//  I/O Usage:
//    Knob A0: Pattern selection (0-255 converted to binary, e.g. "0,1,0,0,1,1,0,1")
//    Knob A1: Synth wavefrom selection (Saw, Tri, Saw+Squ, Silence)
//    Analog In A2: Drum waveform selection, can be CV-controlled (Kick, Snare, Percussion, Silence)
//    Analog In A3: Synth wave pitch, can be CV-controlled (not 1V/oct)
//    Digital Out D0: Pattern output (trigger on every 1 bit)
//    Digital Out D1: Inverted pattern output (trigger on every 0 bit)
//    Clock In: External clock input
//    Analog Out: Unipolar Audio Output
//
//  Input Expander: unused
//  Output Expander: 8 bits of output exposed
//
//  Created:  14 Jan 2018 by TTNM (using 20 Objects' Ardcore template)
//  Modified: 16 Jan 2018 by TTNM (cleaned up code and added new waveforms)
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



  int stepCount = 0;



//  ==================== start of setup() ======================

//  This setup routine should be used in any ArdCore sketch that
//  you choose to write; it sets up the pin usage, and sets up
//  initial state. Failure to properly set up the pin usage may
//  lead to damaging the Arduino hardware, or at the very least
//  cause your program to be unstable.

void setup() 
{

  // if you need to send data back to your computer, you need
  // to open the serial device. Otherwise, comment this line out.
  Serial.begin(9600);
  
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
  // one step ahead on clock pulse. up to 8 steps.
  
  if (clkState == HIGH){
    digitalWrite(digPin[0], LOW);
    digitalWrite(digPin[1], LOW);
    
    //convert int to binary
    int zeros = 8 - String(analogRead(0)/4, BIN).length();
    String valueBin;
    for (int i=0; i<zeros; i++){
      valueBin = valueBin + "0";
    }
    valueBin = valueBin + String(analogRead(0)/4, BIN);
  
    //output binary pattern as trigger sequence
    if(valueBin.charAt(stepCount) == '1'){
      digitalWrite(digPin[0], HIGH);
      triggerDrum(analogRead(2)/256); // play drum sound     
      digitalWrite(digPin[0], LOW);
    }else{
      digitalWrite(digPin[1], HIGH); //D1 outputs inverse of D0
      triggerSynth((analogRead(3)*-1)+1023,analogRead(1)/256); // play synth sound
      digitalWrite(digPin[1], LOW);
    }
  
    stepCount++;
    clkState = LOW;

    if (stepCount > 7){
      stepCount = 0;
    }
    
  }
  

         
  
}

void triggerDrum(int drum){

  int waveDrum[4][60] = { //three drum waveforms plus silence
    {255, 255, 255, 255, 0, 0, 0, 235, 235, 235, 235, 0, 0, 0, 255, 255, 255, 255, 255, 0, 0, 0, 235, 235, 235, 235, 235, 0, 0, 0, 255, 235, 255, 255, 235, 255, 0, 0, 0, 0, 0, 255, 255, 235, 255, 235, 235, 255, 235, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {154, 254, 38, 176, 14, 166, 90, 249, 124, 86, 48, 136, 238, 162, 6, 228, 20, 193, 23, 214, 0, 203, 87, 7, 19, 196, 71, 50, 190, 73, 134, 28, 187, 111, 164, 3, 180, 150, 16, 171, 163, 157, 71, 150, 7, 17, 142, 5, 131, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {255,0,255,0,255,0,255,0,255,0,255,0,255,0},
    {0}
  };
    
  for(int i=0; i < 60; i++){
    dacOutput(waveDrum[drum][i]);
    delayMicroseconds(1000);
  }

}

void triggerSynth(int pitch, int synth){

  int waveSynth[4][60] = {    //three synth waveforms plus silence
    {240,200,160,80,30,0,240,200,160,80,30,0,240,200,160,80,30,0,255,240,200,160,80,30,0,240,200,160,80,30,0,255,240,200,160,80,30,0,},
    {10,50,100,150,200,250,200,150,100,50,10,0,10,50,100,150,200,250,200,150,100,50,10,0,10,50,100,150,200,250,200,150,100,50,10,0,10,50,100,150,200,250,200,150,100,50,10,0},
    {240,200,150,120,80,50,30,10,0,220,220,0,0,220,220,0,0,240,200,150,120,80,50,30,10,0,220,220,0,0,220,220,0,0,240,200,150,120,80,50,30,10,0,220,220,0,0,220,220,0,0},
    {0}
  };
  

  
  for(int i=0; i < 60; i++){
    dacOutput(waveSynth[synth][i]);
    delayMicroseconds(pitch);
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
