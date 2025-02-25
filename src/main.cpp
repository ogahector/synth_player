#include <Arduino.h>
#include <U8g2lib.h>
#include <bitset>
#include <string>
#include <cmath>

//Constants
  const uint32_t interval = 100; //Display update interval

//Pin definitions
//Row select and enable
const int RA0_PIN = D3;
const int RA1_PIN = D6;
const int RA2_PIN = D12;
const int REN_PIN = A5;

//Matrix input and output
const int C0_PIN = A2;
const int C1_PIN = D9;
const int C2_PIN = A6;
const int C3_PIN = D1;
const int OUT_PIN = D11;

//Audio analogue out
const int OUTL_PIN = A4;
const int OUTR_PIN = A3;

//Joystick analogue in
const int JOYY_PIN = A0;
const int JOYX_PIN = A1;

//Output multiplexer bits
const int DEN_BIT = 3;
const int DRST_BIT = 4;
const int HKOW_BIT = 5;
const int HKOE_BIT = 6;

//Display driver object
U8G2_SSD1305_128X32_ADAFRUIT_F_HW_I2C u8g2(U8G2_R0);

// Current Step Size
volatile uint32_t currentStepSize;

std::bitset<4> readCols()
{
  // // Set RA0, RA1, RA2 low
  // digitalWrite(RA0_PIN, LOW);
  // digitalWrite(RA1_PIN, LOW);
  // digitalWrite(RA2_PIN, LOW);
  
  // // Enable Sel
  // digitalWrite(REN_PIN, HIGH);

  std::bitset<4> result;
  
  result[0] = digitalRead(C0_PIN);
  result[1] = digitalRead(C1_PIN);
  result[2] = digitalRead(C2_PIN);
  result[3] = digitalRead(C3_PIN);

  return result;
}

void setRow(uint8_t rowIdx){
  digitalWrite(REN_PIN, LOW);

  // Only leave 3 LSB
  rowIdx = (rowIdx << 5) >> 5;

  // Check unused row, default to 0
  if(rowIdx == 7) rowIdx = 0;

  // Select Row
  digitalWrite(RA0_PIN, (rowIdx) & 1);
  digitalWrite(RA1_PIN, (rowIdx >> 1) & 1);
  digitalWrite(RA2_PIN, (rowIdx >> 2) & 1);

  digitalWrite(REN_PIN, HIGH);
}

std::string inputToKeyString(uint32_t inputs)
{
  // Isolate first 12 LSB
  inputs = (inputs << (32 - 12)) >> (32 - 12);
  
  inputs = !inputs; // one's complement bc active low

  if(inputs & 1)
  return "C";
  else if((inputs >> 1) & 1)
    return "C#";
  else if((inputs >> 2) & 1)
    return "D";
  else if((inputs >> 3) & 1)
    return "D#";
  else if((inputs >> 4) & 1)
    return "E";
  else if((inputs >> 5) & 1)
    return "F";
  else if((inputs >> 6) & 1)
    return "F#";
  else if((inputs >> 7) & 1)
    return "G";
  else if((inputs >> 8) & 1)
    return "G#";
  else if((inputs >> 9) & 1)
    return "A";
  else if((inputs >> 10) & 1)
    return "A#";
  else if((inputs >> 11) & 1)
    return "B";
  else return "NOKEY";
}

void sampleISR()
{
  static uint32_t phaseAcc = 0;
  phaseAcc += currentStepSize;
}

void updateCurStepSize(uint32_t inputs, const uint32_t stepSizes[])
{
  for(size_t i = 0; i < 12; i++)
  {
    if( !((inputs >> i) & 1) ) // check active low i-th bit
    {
      currentStepSize = stepSizes[i];
      return;
    }
  }
  currentStepSize = 0;
}


//Function to set outputs using key matrix
void setOutMuxBit(const uint8_t bitIdx, const bool value) {
      digitalWrite(REN_PIN,LOW);
      digitalWrite(RA0_PIN, bitIdx & 0x01);
      digitalWrite(RA1_PIN, bitIdx & 0x02);
      digitalWrite(RA2_PIN, bitIdx & 0x04);
      digitalWrite(OUT_PIN,value);
      digitalWrite(REN_PIN,HIGH);
      delayMicroseconds(2);
      digitalWrite(REN_PIN,LOW);
}

void setup() {
  // put your setup code here, to run once:

  //Set pin directions
  pinMode(RA0_PIN, OUTPUT);
  pinMode(RA1_PIN, OUTPUT);
  pinMode(RA2_PIN, OUTPUT);
  pinMode(REN_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(OUTL_PIN, OUTPUT);
  pinMode(OUTR_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(C0_PIN, INPUT);
  pinMode(C1_PIN, INPUT);
  pinMode(C2_PIN, INPUT);
  pinMode(C3_PIN, INPUT);
  pinMode(JOYX_PIN, INPUT);
  pinMode(JOYY_PIN, INPUT);

  //Initialise display
  setOutMuxBit(DRST_BIT, LOW);  //Assert display logic reset
  delayMicroseconds(2);
  setOutMuxBit(DRST_BIT, HIGH);  //Release display logic reset
  u8g2.begin();
  setOutMuxBit(DEN_BIT, HIGH);  //Enable display power supply

  //Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t next = millis();
  static uint32_t count = 0;

  while (millis() < next);  //Wait for next interval

  next += interval;
  static std::bitset<32> inputs;  
  
  // Define Step Sizes
  static uint32_t minStepSize = (1 << 32) * 440 / 32000;
  static uint32_t stepSizes [] = { minStepSize };
  for(size_t i = 1; i < 12; i++)
  {
    // stepSizes[i] = stepSizes[i-1] * 1.059463094; // * 12th root of 2
    stepSizes[i] = minStepSize * ((uint32_t) pow(1.059463094, i));
  }

  // Set Rows and Read Columns
  for(uint8_t i = 0; i < 3; i++)
  {
    setRow(i);
    delayMicroseconds(3);
    std::bitset<4> current_column = readCols();
    for(size_t j = 0; j < 4; j++)
    {
      inputs[i*4 + j] = current_column[j];
    }
    
  }

  // Update Current Step Size
  updateCurStepSize(inputs.to_ulong(), stepSizes);

  //Update display
  u8g2.clearBuffer();         // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
  u8g2.drawStr(2,10,"Helllo World!");  // write something to the internal memory
  u8g2.setCursor(2,20);
  u8g2.print(inputs.to_ulong(), HEX); 
  u8g2.drawStr(2, 30, inputToKeyString(inputs.to_ulong()).c_str());
  u8g2.sendBuffer();          // transfer internal memory to the display

  //Toggle LED
  digitalToggle(LED_BUILTIN);
  
}