#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <atomic>
#include <bitset>
#include <string>
#include <cmath>

//Constants
const uint32_t interval = 100; //Display update interval
HardwareTimer sampleTimer(TIM1);

// Multi Note Constants
const uint8_t MAX_POLYPHONY = 4; // Max number of simultaneous notes
volatile uint32_t activeStepSizes[MAX_POLYPHONY] = {0}; 
volatile uint8_t activeNotes = 0; // Number of active notes

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

// TYPEDEFS
class KnobState_t {
private:

public:
  uint8_t currA;
  uint8_t currB;
  uint8_t prevA;
  uint8_t prevB;

  int curr_direction;

  uint32_t count;

  
  KnobState_t() : currA(0), currB(0), prevA(0), prevB(0),
                  curr_direction(0), count(0) {};

  int out()
  {
    if ((currB == 0 && prevB == 0 && prevA == 0 && currA == 1) || (currB == 1 && prevB == 1 && prevA == 1 && currA == 0)) {
      curr_direction = 1;  // Clockwise
    } else if ((currB == 0 && prevB == 0 && prevA == 1 && currA == 0) || (currB == 1 && prevB == 1 && prevA == 0 && currA == 1)) {
      curr_direction = -1; // Counter-clockwise
    } else {
      curr_direction = 0;  // No change or invalid state
    }

    // Handle volume change based on the direction
    if (curr_direction == 1 && count < 7) count++;  // Increase volume
    if (curr_direction == -1 && count > 0) count--; // Decrease volume

    return curr_direction;
  }

  std::string to_str()
  {
    if(curr_direction == 1)
      return "+1 returned";
    else if(curr_direction == -1)
      return "-1 returned";

    return "No Change";
  }
};

struct {
  std::bitset<32> inputs;
  KnobState_t knob3;
  SemaphoreHandle_t mutex;
} sysState;

//Display driver object
U8G2_SSD1305_128X32_ADAFRUIT_F_HW_I2C u8g2(U8G2_R0);

// Current Step Size
volatile uint32_t currentStepSize;


constexpr uint32_t hz2stepSize(float freq)
{
  return (uint32_t) ((4294967296 * freq) / 22000);
}

const uint32_t stepSizes [] = {// 22Hz between each note
  hz2stepSize(242.0),
  hz2stepSize(264.0),
  hz2stepSize(286.0),
  hz2stepSize(308.0),
  hz2stepSize(330.0),
  hz2stepSize(352.0),
  hz2stepSize(374.0),
  hz2stepSize(396.0),
  hz2stepSize(418.0),
  hz2stepSize(440.0),
  hz2stepSize(462.0),
  hz2stepSize(484.0)
};

std::bitset<4> readCols()
{
  std::bitset<4> result;
  
  result[0] = digitalRead(C0_PIN);
  result[1] = digitalRead(C1_PIN);
  result[2] = digitalRead(C2_PIN);
  result[3] = digitalRead(C3_PIN);

  return result;
}

void setRow(uint8_t rowIdx){
  digitalWrite(REN_PIN, LOW);
  
  digitalWrite(RA0_PIN, (rowIdx) & 0b1);
  digitalWrite(RA1_PIN, (rowIdx) & 0b10);
  digitalWrite(RA2_PIN, (rowIdx) & 0b100);

  digitalWrite(REN_PIN, HIGH);
}

std::string inputToKeyString(uint32_t inputs)
{
  // Isolate first 12 LSB
  // inputs = (inputs << (32 - 12)) >> (32 - 12);
  int index;
  for(index = 0; index < 12; index++)
  {
    if(!((inputs >> index) & 1))
      break; 
  }

  switch(index){
    case 0: return "C";
    case 1: return "C#";
    case 2: return "D";
    case 3: return "D#";
    case 4: return "E";
    case 5: return "F";
    case 6: return "F#";
    case 7: return "G";
    case 8: return "G#";
    case 9: return "A";
    case 10: return "A#";
    case 11: return "B";
    default: return "";
  }
  return "";
}

void sampleISR()
{
  static uint32_t phaseAcc[MAX_POLYPHONY] = {0};
  int32_t Vout = 0;

  for (uint8_t i = 0; i < activeNotes; i++) {
    phaseAcc[i] += activeStepSizes[i];
    Vout += ((phaseAcc[i] >> 24) - 128); // Sum waveforms
  }

  Vout = Vout >> (8 - sysState.knob3.count);

  Vout = Vout / max(1, (int)activeNotes);
  analogWrite(OUTR_PIN, Vout + 128);
}

uint32_t state2stepSize(uint32_t inputs)
{
  for(size_t i = 0; i < 12; i++)
  {
    if( !((inputs >> i) & 1) ) // check active low i-th bit
    {
      return stepSizes[i];
    }
  }
  return 0;
}


void scanKeysTask(void * pvParameters) 
{
  const TickType_t xFrequency = 20/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  
  while(1){
    vTaskDelayUntil( &xLastWakeTime, xFrequency );

    activeNotes = 0;

    xSemaphoreTake(sysState.mutex, portMAX_DELAY);

    // NOTES
    for(uint8_t i = 0; i < 7; i++)
    {
      setRow(i);
      delayMicroseconds(3);
      std::bitset<4> current_column = readCols();
      for(int j = 0; j < 4; j++)
      {
        sysState.inputs[i*4 + j] = current_column[j];

        if (i < 3 && activeNotes < MAX_POLYPHONY && current_column[j] == 0) { // If key is pressed
          activeStepSizes[activeNotes++] = stepSizes[i*4 + j]; // Store step size
        }
      }
    }

    uint32_t localCurrentStepSize = state2stepSize(sysState.inputs.to_ulong());

    sysState.knob3.prevA = sysState.knob3.currA;
    sysState.knob3.prevB = sysState.knob3.currB;
    sysState.knob3.currA = sysState.inputs[3 * 4 ]; // Knob 3 A
    sysState.knob3.currB = sysState.inputs[3 * 4 + 1]; // Knob 3 B

    int direction = sysState.knob3.out(); 
    // if (direction == 1 && sysState.volume < 7) sysState.volume++;   // Increase volume
    // if (direction == -1 && sysState.volume > 0) sysState.volume--; // Decrease volume
    // sysState.volume += direction;
    xSemaphoreGive(sysState.mutex);

    __atomic_store_n(&currentStepSize, localCurrentStepSize, __ATOMIC_RELAXED);
  }
}

void displayUpdateTask(void* vParam)
{
  const TickType_t xFrequency = 100/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  while(1)
  {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    u8g2.clearBuffer();         // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
    u8g2.drawStr(2,10,"Current Stack Size: ");  // write something to the internal memory
    u8g2.setCursor(2,20);
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    u8g2.print(sysState.knob3.count, DEC); 
    u8g2.drawStr(2, 30, inputToKeyString(sysState.inputs.to_ulong()).c_str());
    xSemaphoreGive(sysState.mutex);
    u8g2.sendBuffer();          // transfer internal memory to the display

    digitalToggle(LED_BUILTIN);
  }
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

  sampleTimer.setOverflow(22000, HERTZ_FORMAT);
  sampleTimer.attachInterrupt(sampleISR);
  sampleTimer.resume();

  sysState.mutex = xSemaphoreCreateMutex();

  TaskHandle_t scanKeysHandle = NULL;
  xTaskCreate(
    scanKeysTask,		/* Function that implements the task */
    "scanKeys",		/* Text name for the task */
    32,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    1,			/* Task priority */
    &scanKeysHandle /* Pointer to store the task handle */
  );	

  TaskHandle_t updateDisplayHandle = NULL;
  xTaskCreate(
    displayUpdateTask,
    "displayUpdate",
    32,
    NULL,
    2, 
    &updateDisplayHandle
  );
  Serial.print("scanKeysTask Stack Usage: ");
  Serial.println(uxTaskGetStackHighWaterMark(scanKeysHandle));
  Serial.print("displayUpdateTask Stack Usage: ");
  Serial.println(uxTaskGetStackHighWaterMark(updateDisplayHandle));

  vTaskStartScheduler();

}

void loop() 
{

  

}