#include <Arduino.h>
#include <U8g2lib.h>
#include <bitset>
#include <STM32FreeRTOS.h>
#include <knob.h>
#include <ES_CAN.h>
#include <doom.h>



// Octave Logic (ideally, the octave would be determined based on the relative location of a keyboard to its neighbours
// but the CAN bus does not contain directional data, so there is no way to know which keyboard is where with respect to the others
// Change this value for each keyboard: 0 for default, +1 for one octave higher, -1 for one octave lower

// Multi Note Constants
const uint8_t MAX_POLYPHONY = 12; // Max number of simultaneous notes
volatile uint32_t activeStepSizes[12] = {0};//Has one for each key

// Show DOOM loading screen
bool doomLoadingShown = false;

//Constants
  const uint32_t interval = 100; //Display update interval
  constexpr uint32_t hz2stepSize(float freq)
  {
    return (uint32_t) ((4294967296 * freq) / 22000);
  }
  const uint32_t stepSizes[] = { //22kHz between each node
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

//Global variables
  struct {
    std::bitset<32> inputs;
    int Volume;
    bool mute = false;
    bool slave = false;
    SemaphoreHandle_t mutex;
    uint8_t RX_Message[8];   
    int Octave = 0;
    bool doomMode = false;
    } sysState;
  QueueHandle_t msgInQ;
  QueueHandle_t msgOutQ;
  SemaphoreHandle_t CAN_TX_Semaphore;
  

//Display driver object
U8G2_SSD1305_128X32_ADAFRUIT_F_HW_I2C u8g2(U8G2_R0);

void renderDoomScene() {
  // Only show loading screen once per activation of doom mode
  if (!doomLoadingShown) {
    u8g2.clearBuffer();
    u8g2.setDrawColor(1);  // 1 = white, 0 = black

    // Render the loading image (from doomLoadScreen)
    for (size_t i = 0; i < numLoadOnes; i++) {
      u8g2.drawPixel(doomLoadScreen[i].col, doomLoadScreen[i].row);
    }
    u8g2.sendBuffer();

    // Wait for 2 seconds
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Mark the loading screen as shown
    doomLoadingShown = true;
  }

  // Always display the start scene
  u8g2.clearBuffer();
  for (size_t i = 0; i < numStartOnes; i++) {
    u8g2.drawPixel(doomStartScene[i].col, doomStartScene[i].row);
  }
  u8g2.sendBuffer();
}



//Hardware Timer
HardwareTimer sampleTimer(TIM1);




std::bitset<4> readCols(){
  std::bitset<4> result;
  result[0] = digitalRead(C0_PIN);
  result[1] = digitalRead(C1_PIN); 
  result[2] = digitalRead(C2_PIN);
  result[3] = digitalRead(C3_PIN);
  return result;
  }

void setRow(uint8_t rowIdx){
  digitalWrite(REN_PIN,LOW);
  digitalWrite(RA0_PIN, (rowIdx & 0x01) ? HIGH : LOW);
  digitalWrite(RA1_PIN, (rowIdx & 0x02) ? HIGH : LOW);
  digitalWrite(RA2_PIN, (rowIdx & 0x04) ? HIGH : LOW);
  digitalWrite(REN_PIN,HIGH);
  }

void sampleISR(){
  static uint32_t phaseAcc[MAX_POLYPHONY] = {0};
  static int32_t Vout = 0;
  uint8_t counter = 0;//Counts the number of active notes
  for (uint8_t i = 0; i < 12; i++) {//Iterates through all keys
    if (activeStepSizes[i] != 0 && counter < MAX_POLYPHONY){
      phaseAcc[counter] += activeStepSizes[i];//Phase accumaltor has channels now
      Vout += ((phaseAcc[counter] >> 24) - 128);
      counter++;
    }
  }
  //NOTE : with above, the first keys pressed from left to right will take preference when there are more than 4 keys pressed
  //This is because the counter will not increment past 4, and the phaseAcc will not be updated for the other keys
  //This is a limitation of the current implementation, may want to change this in the future
  int volume;
  bool mute;
  __atomic_load(&sysState.Volume, &volume, __ATOMIC_RELAXED);
  __atomic_load(&sysState.mute, &mute, __ATOMIC_RELAXED);
  if (mute) Vout = -128;
  else{
    Vout = Vout >> (7 - volume);
    //Vout = Vout / max(1, (int)counter);//Reduces output slightly with extra keys
  }
  analogWrite(OUTR_PIN, Vout + 128);
}


void CAN_RX_ISR (void) {//Recieving CAN ISR
	uint8_t RX_Message_ISR[8];
	uint32_t ID;
	CAN_RX(ID, RX_Message_ISR);
	xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
}

void CAN_TX_ISR (void) {//Transmitting can ISR
	xSemaphoreGiveFromISR(CAN_TX_Semaphore, NULL);
}

std::string inputToKeyString(uint32_t inputs)//Just gets key from input
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

void scanKeysTask(void * pvParameters) {
  const TickType_t xFrequency = 20/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  knob K3 = knob(0,8);//Volume knob
  knob K2 = knob(-4,4);//Octave knob
  bool muteReleased = true;
  bool slaveReleased = true;
  bool prevDoomButton = false;
  std::bitset<32> previousInputs;
  std::bitset<4> cols;
  uint8_t TX_Message[8] = {0};//Message sent over CAN
  while(1){
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    previousInputs = sysState.inputs;
    
    for (int i = 0; i < 7; i++){//Read rows
      setRow(i);
      delayMicroseconds(3);
      cols = readCols();
      for (int j = 0; j < 4; j++) sysState.inputs[4*i + j] = cols[j];
    }


    for (int i = 0; i < 12; i++){//Checks all keys
      if (sysState.inputs[i] != previousInputs[i]){//Checks if a NEW key has been pressed
        TX_Message[0] = (sysState.inputs[i] & 0b1) ? 'R' : 'P';
        TX_Message[1] = sysState.Octave + 4;
        TX_Message[2] = i;
        TX_Message[3] = sysState.mute ? 255 : sysState.Volume;
        xQueueSend( msgOutQ, TX_Message, portMAX_DELAY);//Sends via CAN
      }
    }

    if (!sysState.slave) sysState.Volume = K3.update(sysState.inputs[12], sysState.inputs[13]);//Volume adjustment
    sysState.Octave = K2.update(sysState.inputs[14], sysState.inputs[15]);//Octave Adjustment
    
    //Toggles mute (Knob 3 Press)
    if(!sysState.inputs[21] && muteReleased) {
      muteReleased = false;
      sysState.mute = !sysState.mute;
    }
    else if (sysState.inputs[21]) muteReleased = true;

    //Toggles slave (Knob 2 Press)
    if(!sysState.inputs[20] && slaveReleased) {
      slaveReleased = false;
      sysState.slave = !sysState.slave;
    }
    else if (sysState.inputs[20]) slaveReleased = true;

    //Toggles DOOM (Knob 0 Press)
    if(!sysState.inputs[24] && prevDoomButton) {
      prevDoomButton = false;
      sysState.doomMode = !sysState.doomMode;
      if (sysState.doomMode) {
        doomLoadingShown = false;  // Reset flag when entering doom mode
      }
    }
    else if (sysState.inputs[24]) prevDoomButton = true;

    xSemaphoreGive(sysState.mutex);
  }
}


void displayUpdateTask(void* vParam)
{
  const TickType_t xFrequency = 100/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();

  // Set up bar graph parameters
  const int barWidth = 6;    // Width of each volume bar
  const int barHeight = 8;  // Height of the volume bar
  const int numBars = 7;     // Maximum volume level (0-7)
  const int barStartX = 62; // X position where the bars start
  const int barStartY = 12;  // Y position where the bars are drawn


  while(1)
  {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    if (sysState.doomMode) {
      xSemaphoreGive(sysState.mutex);
      renderDoomScene();
      continue;  // Skip normal UI update.
    }
    u8g2.clearBuffer();         // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
    // u8g2.drawStr(2,10,"Current Stack Size: ");  // write something to the internal memory
    u8g2.drawStr(2, 10, "Octave: ");
    u8g2.setCursor(55,10);
    u8g2.print(sysState.Octave, DEC);
    u8g2.setCursor(75,10);
    if(sysState.slave) u8g2.print("Slave");
    else u8g2.print("Master");

    u8g2.drawStr(2, 20, "Volume: ");
    u8g2.setCursor(55,20);
    
    if (sysState.mute) u8g2.print("X");
    else u8g2.print(sysState.Volume, DEC); 
    
    int volumeLevel = sysState.Volume;  // Get the current volume count (0-7)
    for (int i = 0; i < volumeLevel+1; i++) {
      u8g2.drawBox(barStartX + i * (barWidth + 2), barStartY, barWidth, barHeight);  // Draw the bar
    }

    u8g2.setCursor(65,30);
    u8g2.print((char) sysState.RX_Message[0]);
    u8g2.print(sysState.RX_Message[1]);
    u8g2.print(sysState.RX_Message[2]);
    u8g2.print(sysState.RX_Message[3]);

    u8g2.drawStr(2, 30, inputToKeyString(sysState.inputs.to_ulong()).c_str());
    xSemaphoreGive(sysState.mutex);
    u8g2.sendBuffer();          // transfer internal memory to the display

    digitalToggle(LED_BUILTIN);
  }
}

void decodeTask(void * pvParameters){
  uint32_t activeStepSizesLocal[12];
  uint8_t RX_Message[8];
  while (1){
    xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);//Gets current message from queue
    //__atomic_load(&activeStepSizes, &activeStepSizesLocal, __ATOMIC_RELAXED);
    if (RX_Message[0] == 'R'){//Key is reset if released
      activeStepSizes[RX_Message[2]] = 0;
    }
    else if (RX_Message[0] == 'P'){//Key is shifted if pressed
      if (RX_Message[1] - 4 >= 0) activeStepSizes[RX_Message[2]] = stepSizes[RX_Message[2]] << (RX_Message[1] - 4);
      else activeStepSizes[RX_Message[2]] = stepSizes[RX_Message[2]] >> abs(RX_Message[1] - 4);
    }
    //__atomic_store_n(&activeStepSizes, &activeStepSizesLocal, __ATOMIC_RELAXED);
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    if (sysState.slave) {//Handles slave muting
      if (RX_Message[3] == 255) sysState.mute = true;
      else {
        sysState.mute = false;
        sysState.Volume = RX_Message[3];
      }
    }
    for (int i = 0; i < 8; i++) sysState.RX_Message[i] = RX_Message[i];//Saves message for printing
    xSemaphoreGive(sysState.mutex);
  }
}

void transmitTask (void * pvParameters) {//Transmits message across CAN bus
	uint8_t msgOut[8];
	while (1) {
		xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
		xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);
		CAN_TX(0x123, msgOut);
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

  //Initialise timer
  sampleTimer.setOverflow(22000, HERTZ_FORMAT);
  sampleTimer.attachInterrupt(sampleISR);
  sampleTimer.resume();

  //Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");

  //Initialise Queue
  msgInQ = xQueueCreate(36,8);
  msgOutQ = xQueueCreate(36,8);

  //Initialise Mutex
  sysState.mutex = xSemaphoreCreateMutex();

  //Initialise Semaphore
  CAN_TX_Semaphore = xSemaphoreCreateCounting(3,3);

  //Initialise CAN bus
  CAN_Init(true);
  setCANFilter(0x123,0x7ff);
  CAN_RegisterRX_ISR(CAN_RX_ISR);
  CAN_RegisterTX_ISR(CAN_TX_ISR);
  CAN_Start();

  //Initialise threads
  TaskHandle_t scanKeysHandle = NULL;
  xTaskCreate(
    scanKeysTask,		/* Function that implements the task */
    "scanKeys",		/* Text name for the task */
    64,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    3,			/* Task priority */
    &scanKeysHandle /* Pointer to store the task handle */
  );	

  TaskHandle_t displayUpdateHandle = NULL;
  xTaskCreate(
    displayUpdateTask,		/* Function that implements the task */
    "displayUpdate",		/* Text name for the task */
    256,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    1,			/* Task priority */
    &displayUpdateHandle /* Pointer to store the task handle */
  );
  
  TaskHandle_t decodeHandle = NULL;
  xTaskCreate(
    decodeTask,		/* Function that implements the task */
    "decode",		/* Text name for the task */
    64,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    2,			/* Task priority */
    &decodeHandle /* Pointer to store the task handle */
  );

  TaskHandle_t transmitHandle = NULL;
  xTaskCreate(
    transmitTask,		/* Function that implements the task */
    "transmit",		/* Text name for the task */
    64,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    2,			/* Task priority */
    &transmitHandle /* Pointer to store the task handle */
  );

  //Start threads
  vTaskStartScheduler();
}

void loop() {

}