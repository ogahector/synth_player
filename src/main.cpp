#include <Arduino.h>
#include <U8g2lib.h>
#include <bitset>
#include <STM32FreeRTOS.h>
#include <globals.h>

#include <CANTask.h>
#include <ISR.h>
#include <ES_CAN.h>
#include <test_tasks.h>

#ifndef TEST_SCANKEYS
  #include <scanKeysTask.h>
#endif

#ifndef TEST_DISPLAYUPDATE
  #include <displayUpdateTask.h>
#endif




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

  sysState.activityList = HOME;

  //Initialise Semaphore
  CAN_TX_Semaphore = xSemaphoreCreateCounting(3,3);

  //Initialise CAN bus
  CAN_Init(true);
  setCANFilter(0x123,0x7ff);
  CAN_RegisterRX_ISR(CAN_RX_ISR);
  CAN_RegisterTX_ISR(CAN_TX_ISR);
  CAN_Start();

  double startTime;
  double elapsed;
  double average;
 

  #ifndef DISABLE_THREADS

  //Initialise threads
  TaskHandle_t scanKeysHandle = NULL;
  xTaskCreate(
    scanKeysTask,		/* Function that implements the task */
    "scanKeys",		/* Text name for the task */
    256,      		/* Stack size in words, not bytes */
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
  #endif

  #ifdef DISABLE_THREADS
    double T_scanKeys = 0; 
    double T_displayUpdate = 0;
    double T_decode = 0;
    double T_transmit =0;
  #endif

  #ifdef TEST_SCANKEYS
    Serial.println("Testing worst-case execution time for scanKeysTask HOME state");
    startTime = micros();
    for (int iter = 0; iter < 32; iter++) {
      testScanKeys(0);
    }
    elapsed = micros() - startTime;
    Serial.print("32 iterations of scanKeysTask_Test took: ");
    Serial.print(elapsed);
    Serial.println(" microseconds");
    Serial.print("Average per iteration: ");
    average = elapsed / 32.0;
    Serial.print(average);
    Serial.println(" microseconds");
    if (average>T_scanKeys){
      T_scanKeys = average;
    }
    Serial.println("Testing worst-case execution time for scanKeysTask MENU state");
    startTime = micros();
    for (int iter = 0; iter < 32; iter++) {
      testScanKeys(1);
    }
    elapsed = micros() - startTime;
    Serial.print("32 iterations of scanKeysTask_Test took: ");
    Serial.print(elapsed);
    Serial.println(" microseconds");
    Serial.print("Average per iteration: ");
    average = elapsed / 32.0;
    Serial.print(average);
    Serial.println(" microseconds");
    if (average>T_scanKeys){
      T_scanKeys = average;
    }

    Serial.println("Testing worst-case execution time for scanKeysTask DOOM state");
    startTime = micros();
    for (int iter = 0; iter < 32; iter++) {
      testScanKeys(2);
    }
    elapsed = micros() - startTime;
    Serial.print("32 iterations of scanKeysTask_Test took: ");
    Serial.print(elapsed);
    Serial.println(" microseconds");
    Serial.print("Average per iteration: ");
    average = elapsed / 32.0;
    Serial.print(average);
    Serial.println(" microseconds");
    if (average>T_scanKeys){
      T_scanKeys = average;
    }

    Serial.println("Testing worst-case execution time for scanKeysTask WAVES state");
    startTime = micros();
    for (int iter = 0; iter < 32; iter++) {
      testScanKeys(3);
    }
    elapsed = micros() - startTime;
    Serial.print("32 iterations of scanKeysTask_Test took: ");
    Serial.print(elapsed);
    Serial.println(" microseconds");
    Serial.print("Average per iteration: ");
    average = elapsed / 32.0;
    Serial.print(average);
    Serial.println(" microseconds");
    if (average>T_scanKeys){
      T_scanKeys = average;
    }

  #endif

  #ifdef TEST_DISPLAYUPDATE
    Serial.println("Testing worst-case execution time for displayUpdate HOME state");
    startTime = micros();
    for (int iter = 0; iter < 32; iter++) {
      testDisplayUpdate(0);
    }
    elapsed = micros() - startTime;
    Serial.print("32 iterations of displayUpdate_Test took: ");
    Serial.print(elapsed);
    Serial.println(" microseconds");
    Serial.print("Average per iteration: ");
    average = elapsed / 32.0;
    Serial.print(average);
    Serial.println(" microseconds");
    if (T_displayUpdate>average){
      average = T_displayUpdate;
    }

    Serial.println("Testing worst-case execution time for displayUpdate MENU state");
    startTime = micros();
    for (int iter = 0; iter < 32; iter++) {
      testDisplayUpdate(1);
    }
    elapsed = micros() - startTime;
    Serial.print("32 iterations of displayUpdate_Test took: ");
    Serial.print(elapsed);
    Serial.println(" microseconds");
    Serial.print("Average per iteration: ");
    average = elapsed / 32.0;
    Serial.print(average);
    Serial.println(" microseconds");
    if (T_displayUpdate>average){
      average = T_displayUpdate;
    }
    Serial.println("Testing worst-case execution time for displayUpdate DOOM state");
    startTime = micros();
    for (int iter = 0; iter < 32; iter++) {
      testDisplayUpdate(2);
    }
    elapsed = micros() - startTime;
    Serial.print("32 iterations of displayUpdate_Test took: ");
    Serial.print(elapsed);
    Serial.println(" microseconds");
    Serial.print("Average per iteration: ");
    average = elapsed / 32.0;
    Serial.print(average);
    Serial.println(" microseconds");
    if (T_displayUpdate>average){
      average = T_displayUpdate;
    }

    Serial.println("Testing worst-case execution time for displayUpdate WAVES state");
    startTime = micros();
    for (int iter = 0; iter < 32; iter++) {
      testDisplayUpdate(3);
    }
    elapsed = micros() - startTime;
    Serial.print("32 iterations of displayUpdate_Test took: ");
    Serial.print(elapsed);
    Serial.println(" microseconds");
    Serial.print("Average per iteration: ");
    average = elapsed / 32.0;
    Serial.print(average);
    Serial.println(" microseconds");
    if (T_displayUpdate>average){
      average = T_displayUpdate;
    }
    
  #endif

  #ifdef DISABLE_THREADS
  double period_tau_i = 10000.0;
  double tau_n=10000.0;

  double L_n = (tau_n / period_tau_i) * T_scanKeys +
             (tau_n / period_tau_i) * T_decode +
             (tau_n /period_tau_i) * T_transmit;

  Serial.print("Worst-case latency L_n = ");
  Serial.print(L_n);
  Serial.println(" microseconds");
  #endif
  //Start threads
  #ifndef DISABLE_THREADS
  vTaskStartScheduler();
  #endif
}

void loop() {
}