/*
  Clocky for Arduino Nano
  Midi and DIN Sync outputter
  March 2022 - Travis Thatcher recompas@gmail.com
*/
 
 
/************** EDIT HERE ****************/
#include <MIDI.h>
#include <arduino-timer.h>

// Created and binds the MIDI interface to the default hardware Serial port
MIDI_CREATE_DEFAULT_INSTANCE();
 
static boolean test = false; // for debugging
int startTime = 0;

// MIDI channel to send messages to
const int channelOut = 1;

// timers
auto clockTimer = timer_create_default();
auto dinClockTimer  = timer_create_default();

/************ END EDIT HERE **************/

//average count
const int numReadings = 20;

//analog readings vars
int idx=0;
int readings0[numReadings];
int readings1[numReadings];

int pot0Total = 0;
int pot1Total = 0;

int pot0Avg = 0;
int pot1Avg = 0;

int pot0 = A1;
int pot1 = A2;


float ppqMs = 0;

long lastDebounceTime = 0;
long debounceDelay = 50;

float tempoScaled = 0;

boolean playing = false;
boolean ticking = false;

float swingOffset = 0;

// 24/qn count
int currentDiv = 0;

int prevToggleRead = 0;

int toggleRead = 0;
const int BUTTONPIN = 3;
const int DINCLOCKPIN = 5;
const int DINSTARTPIN = 7;
const int CLOCKLED = 9;
const int ANALOGCLOCKPIN = 12;

void setup() {

  

 startTime = millis();
  pinMode(BUTTONPIN, INPUT); // button
  pinMode(DINCLOCKPIN, OUTPUT); // din clock
  digitalWrite(DINCLOCKPIN, LOW);
  pinMode(DINSTARTPIN, OUTPUT); // din start/stop
  digitalWrite(DINSTARTPIN, LOW);
  pinMode(CLOCKLED, OUTPUT); // quarter note clock led
  digitalWrite(CLOCKLED, LOW);
  pinMode(ANALOGCLOCKPIN, OUTPUT); // 16th note clock out
  digitalWrite(ANALOGCLOCKPIN, LOW);
 
  if(test){
    Serial.begin(9600); 
    Serial.println("Hi!"); 
  }
  else{
    MIDI.begin(1); 
    MIDI.turnThruOff();
  }
  readings0[0]=0;
  readings1[0]=0;

}

void dinClockOff(){
  dinClockTimer.cancel();
 
  digitalWrite(DINCLOCKPIN, LOW);
  return false;
}

void handleTick(){
  
  clockTimer.cancel();
  ticking = false;
  if(currentDiv%6==0){
    digitalWrite(ANALOGCLOCKPIN, HIGH);
  }else{
    digitalWrite(ANALOGCLOCKPIN, LOW);
  }
  currentDiv++;
  if (currentDiv>23){
    currentDiv = 0;
  }
  if(test){
    //Serial.println("tick yo");
  }else{   
    // send midi clock message
    MIDI.sendRealTime(midi::Clock);
  }
  digitalWrite(DINCLOCKPIN, HIGH);
  dinClockTimer.in(ppqMs/2, dinClockOff);
  return false;
}

void toggleState() {
  
  if (!playing) {
    if(!test){
      MIDI.sendRealTime(midi::Start);
    }else{
      Serial.println("play");
    }
    currentDiv = 0;
    digitalWrite(DINSTARTPIN, HIGH);
    playing = true;
    handleTick();
   } else {
      
      currentDiv = 0;
      if(!test){
        MIDI.sendRealTime(midi::Stop);
      }else{
        Serial.println("stop");
      }
      digitalWrite(DINSTARTPIN, LOW);
      digitalWrite(DINCLOCKPIN, LOW);
      playing = false;
    
   }
  
  
}

void loop() {

  // handle the start/stop button
  toggleRead = digitalRead(BUTTONPIN);

  if(toggleRead == prevToggleRead) {
    lastDebounceTime = millis();
    prevToggleRead = toggleRead;
  }else if ((millis() - lastDebounceTime) > debounceDelay) {
    if(toggleRead!=0){
      toggleState();   
    }
    prevToggleRead = toggleRead;
  }
  
  
  // average potentiometer the readings!

  readings0[idx] = analogRead(pot0);
  readings1[idx] = analogRead(pot1);
 
  idx++;
  
  if(idx >= numReadings){
    idx=0;
  }
  pot0Total = pot1Total = 0;
  for(int j=0; j<numReadings; j++){
    pot0Total = readings0[j] + pot0Total;
    pot1Total = readings1[j] + pot1Total;
  }
  pot0Avg = pot0Total / numReadings;
  pot1Avg = pot1Total / numReadings;

  //if clock change
  // based on lowest speed of 60bpm = 1 beat per sec, 41.66667 ppq time)
  // calculate swingOffset
  
  float bpm = (pot0Avg/1023.0)*300+60;
  
  ppqMs =  (60000/bpm)/24;
  
  swingOffset = ((pot1Avg/1023.0)*(ppqMs));
  swingOffset = swingOffset*.4;

  

  
  if((!ticking)&&(playing==true)){
    if(test){
      if(currentDiv%6==0){
        Serial.println(swingOffset);
        Serial.println(ppqMs);
        Serial.println(bpm);
      }
      
    }
    ticking = true;
    // 16th delay
    // scheme: move 2nd and 4th 16th time

    
    if((currentDiv==6)){
      clockTimer.in(ppqMs+(swingOffset*6), handleTick);
    }else if((currentDiv>=7)&&(currentDiv<12)){
      clockTimer.in(ppqMs-swingOffset, handleTick);
    }else
    if((currentDiv==18)){
      clockTimer.in(ppqMs+(swingOffset*6), handleTick);
    }else if((currentDiv>=19)&&(currentDiv<23)){
      clockTimer.in(ppqMs-swingOffset, handleTick);
    }else{ 
      clockTimer.in(ppqMs, handleTick);
    }


    if(currentDiv < 5){
      digitalWrite(CLOCKLED, HIGH);
    }else{
      digitalWrite(CLOCKLED, LOW);
    } 
    
    
  }
  
  clockTimer.tick();
  dinClockTimer.tick();
  //handleTick();
  
}
