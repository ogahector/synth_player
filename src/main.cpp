// #include <Arduino.h>
#include <globals.h>
#include <U8g2lib.h>
#include <bitset>
#include <STM32FreeRTOS.h>

#include <sig_gen.h>
#include <CANTask.h>
#include <ISR.h>
#include <ES_CAN.h>
#include <test_tasks.h>
#include <recordTask.h>

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

static void TIM6_Init();
static void DMA_Init();
static void ADC_Init();
static void DAC_Init();
static void GPIO_Init();


void setup() {
  // put your setup code here, to run once:

  // //Set pin directions
  pinMode(RA0_PIN, OUTPUT);
  pinMode(RA1_PIN, OUTPUT);
  pinMode(RA2_PIN, OUTPUT);
  pinMode(REN_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(OUTL_PIN, OUTPUT);
  // pinMode(OUTR_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(C0_PIN, INPUT);
  pinMode(C1_PIN, INPUT);
  pinMode(C2_PIN, INPUT);
  pinMode(C3_PIN, INPUT);
  // pinMode(JOYX_PIN, INPUT);
  // pinMode(JOYY_PIN, INPUT);

  //Initialise display
  setOutMuxBit(DRST_BIT, LOW);  //Assert display logic reset
  delayMicroseconds(2);
  setOutMuxBit(DRST_BIT, HIGH);  //Release display logic reset
  u8g2.begin();
  setOutMuxBit(DEN_BIT, HIGH);  //Enable display power supply

  // Initialise HAL
  // HAL_Init();

  // Initialise SysClock
  // SystemClock_Config();

  //Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");


  // Assert wave_buffer as 0s initially
  memset((uint32_t*) dac_buffer, 0UL, DAC_BUFFER_SIZE);


  //Initialise GPIO
  GPIO_Init();

  //Initialise DMA
  DMA_Init();

  //Initialise DAC
  DAC_Init();

  //Initliase ADC
  ADC_Init();

  //Initialise timer
  TIM6_Init();

  HAL_Delay(1);

  HAL_TIM_Base_Start(&htim6);

  //Initialise Queue
  msgInQ = xQueueCreate(36,8);
  msgOutQ = xQueueCreate(36,8);

 

  //Initialise Mutex
  sysState.mutex = xSemaphoreCreateMutex();

  sysState.activityList = HOME;

  //Initialise Record Mutex
  record.mutex = xSemaphoreCreateMutex();

  // Initialise Voices Mutex
  voices.mutex = xSemaphoreCreateMutex();

  //Initialise CAN Semaphore
  CAN_TX_Semaphore = xSemaphoreCreateCounting(3,3);

  //Initialise signal semaphore
  signalBufferSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(signalBufferSemaphore);

  //Initialise CAN bus
  CAN_Init(LOOPBACK);
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
    "scanKeys",		/* Text name for the task */  		/* Stack size in words, not bytes */
    103,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    2,			/* Task priority */
    &scanKeysHandle /* Pointer to store the task handle */
  );

  TaskHandle_t displayUpdateHandle = NULL;
  xTaskCreate(
    displayUpdateTask,		/* Function that implements the task */
    "displayUpdate",		/* Text name for the task */
    209,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    1,			/* Task priority */
    &displayUpdateHandle /* Pointer to store the task handle */
  );
  
  TaskHandle_t decodeHandle = NULL;
  xTaskCreate(
    decodeTask,		/* Function that implements the task */
    "decode",		/* Text name for the task */
    58,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    2,			/* Task priority */
    &decodeHandle /* Pointer to store the task handle */
  );

  TaskHandle_t transmitHandle = NULL;
  xTaskCreate(
    transmitTask,		/* Function that implements the task */
    "transmit",		/* Text name for the task */
    512,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    3,			/* Task priority */
    &transmitHandle /* Pointer to store the task handle */
  );

  TaskHandle_t signalHandle = NULL;
  xTaskCreate(
    signalGenTask,		/* Function that implements the task */
    "signal",		/* Text name for the task */
    55,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    1,			/* Task priority */
    &signalHandle /* Pointer to store the task handle */
  );

  TaskHandle_t recordHandle = NULL;
  xTaskCreate(
    recordTask,		/* Function that implements the task */
    "record",		/* Text name for the task */
    512,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    2,			/* Task priority */
    &recordHandle /* Pointer to store the task handle */
  );

  //Initalise voices 
  for (int octave = 0; octave < 9; octave++){
    for (int key = 0; key < 12; key++){
      if (octave < 4){
        voices.voices_array[octave * 12 + key].phaseInc = stepSizes[key] >> abs(octave - 4);
      }
      else{
        voices.voices_array[octave * 12 + key].phaseInc = stepSizes[key] << (octave - 4);
      }
    }
  }

  Serial.println("Setup complete, starting scheduler...");
  #endif

  #ifdef DISABLE_THREADS
    double T_scanKeys = 0; 
    double T_displayUpdate = 0;
    double T_decode = 0;
    double T_transmit = 0;
    double T_sig = 0;
    double T_record = 0;
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

    Serial.println("Testing worst-case execution time for scanKeysTask RECORDING state");
    startTime = micros();
    for (int iter = 0; iter < 32; iter++) {
      testScanKeys(4);
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
    if (T_displayUpdate < average){
      T_displayUpdate = average;
    }

    // Serial.println("Testing worst-case execution time for displayUpdate MENU state");
    // startTime = micros();
    // for (int iter = 0; iter < 32; iter++) {
    //   testDisplayUpdate(1);
    // }
    // elapsed = micros() - startTime;
    // Serial.print("32 iterations of displayUpdate_Test took: ");
    // Serial.print(elapsed);
    // Serial.println(" microseconds");
    // Serial.print("Average per iteration: ");
    // average = elapsed / 32.0;
    // Serial.print(average);
    // Serial.println(" microseconds");
    // Menu will be higher since it needs to animate the slide
    // if (T_displayUpdate < average){
    //   T_displayUpdate = average;
    // }

    //DOOM removed from testing
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
    if (T_displayUpdate < average){
      T_displayUpdate = average;
    }

    Serial.println("Testing worst-case execution time for displayUpdate RECORDING state");
    startTime = micros();
    for (int iter = 0; iter < 32; iter++) {
      testDisplayUpdate(4);
    }
    elapsed = micros() - startTime;
    Serial.print("32 iterations of displayUpdate_Test took: ");
    Serial.print(elapsed);
    Serial.println(" microseconds");
    Serial.print("Average per iteration: ");
    average = elapsed / 32.0;
    Serial.print(average);
    Serial.println(" microseconds");
    if (T_displayUpdate < average){
      T_displayUpdate = average;
    }
    
  #endif

  #ifdef TEST_DECODE
  Serial.println("Testing worst-case execution time for decode");
  startTime = micros();
  for (int iter = 0; iter < 32; iter++) {
    testDecode();
  }
  elapsed = micros() - startTime;
  Serial.print("32 iterations of decode test took: ");
  Serial.print(elapsed);
  Serial.println(" microseconds");
  Serial.print("Average per iteration: ");
  average = elapsed / 32.0;
  Serial.print(average);
  Serial.println(" microseconds");
  if (T_decode < average){
    T_decode = average;
  }
  #endif

  #ifdef TEST_TRANSMIT
  Serial.println("Testing worst-case execution time for transmit");
  startTime = micros();
  for (int iter = 0; iter < 32; iter++) {
    testTransmit();
  }
  elapsed = micros() - startTime;
  Serial.print("32 iterations of transmit test took: ");
  Serial.print(elapsed);
  Serial.println(" microseconds");
  Serial.print("Average per iteration: ");
  average = elapsed / 32.0;
  Serial.print(average);
  Serial.println(" microseconds");
  if (T_transmit < average){
    T_transmit = average;
  }
  #endif

  #ifdef TEST_SIGGEN
  Serial.println("Testing worst-case execution time for Sig Gen (Precompute)");
  startTime = micros();
  for (int iter = 0; iter < 32; iter++) {
    computeValues();
  }
  elapsed = micros() - startTime;
  Serial.print("32 iterations of precompute test took: ");
  Serial.print(elapsed);
  Serial.println(" microseconds");
  Serial.print("Average per iteration: ");
  average = elapsed / 32.0;
  Serial.print(average);
  Serial.println(" microseconds");

  Serial.println("Testing worst-case execution time for Sig Gen (SINE)");
  startTime = micros();
  for (int iter = 0; iter < 32; iter++) {
    testSigGen(0);
  }
  elapsed = micros() - startTime;
  Serial.print("32 iterations of sig gen test took: ");
  Serial.print(elapsed);
  Serial.println(" microseconds");
  Serial.print("Average per iteration: ");
  average = elapsed / 32.0;
  Serial.print(average);
  Serial.println(" microseconds");
  if (T_sig < average){
    T_sig = average;
  }

  Serial.println("Testing worst-case execution time for Sig Gen (SQUARE)");
  startTime = micros();
  for (int iter = 0; iter < 32; iter++) {
    testSigGen(1);
  }
  elapsed = micros() - startTime;
  Serial.print("32 iterations of sig gen test took: ");
  Serial.print(elapsed);
  Serial.println(" microseconds");
  Serial.print("Average per iteration: ");
  average = elapsed / 32.0;
  Serial.print(average);
  Serial.println(" microseconds");
  if (T_sig < average){
    T_sig = average;
  }

  Serial.println("Testing worst-case execution time for Sig Gen (SAWTOOTH)");
  startTime = micros();
  for (int iter = 0; iter < 32; iter++) {
    testSigGen(2);
  }
  elapsed = micros() - startTime;
  Serial.print("32 iterations of sig gen test took: ");
  Serial.print(elapsed);
  Serial.println(" microseconds");
  Serial.print("Average per iteration: ");
  average = elapsed / 32.0;
  Serial.print(average);
  Serial.println(" microseconds");
  if (T_sig < average){
    T_sig = average;
  }

  Serial.println("Testing worst-case execution time for Sig Gen (TRIANGLE)");
  startTime = micros();
  for (int iter = 0; iter < 32; iter++) {
    testSigGen(3);
  }
  elapsed = micros() - startTime;
  Serial.print("32 iterations of sig gen test took: ");
  Serial.print(elapsed);
  Serial.println(" microseconds");
  Serial.print("Average per iteration: ");
  average = elapsed / 32.0;
  Serial.print(average);
  Serial.println(" microseconds");
  if (T_sig < average){
    T_sig = average;
  }
  #endif

  #ifdef TEST_RECORD

  Serial.println("Testing worst-case execution time for record (Recording only)");
  startTime = micros();
  for (int iter = 0; iter < 32; iter++) {
    testRecord(0);
  }
  elapsed = micros() - startTime;
  Serial.print("32 iterations of recording test took: ");
  Serial.print(elapsed);
  Serial.println(" microseconds");
  Serial.print("Average per iteration: ");
  average = elapsed / 32.0;
  Serial.print(average);
  Serial.println(" microseconds");
  if (T_record < average){
    T_record = average;
  }

  fillPLayback();
  Serial.println("Testing worst-case execution time for record (Playback only)");
  startTime = micros();
  for (int iter = 0; iter < 32; iter++) {
    testRecord(1);
  }
  elapsed = micros() - startTime;
  Serial.print("32 iterations of playback test took: ");
  Serial.print(elapsed);
  Serial.println(" microseconds");
  Serial.print("Average per iteration: ");
  average = elapsed / 32.0;
  Serial.print(average);
  Serial.println(" microseconds");
  if (T_record < average){
    T_record = average;
  }

  Serial.println("Testing worst-case execution time for record (Recording and Playback)");
  startTime = micros();
  for (int iter = 0; iter < 32; iter++) {
    testRecord(1);
  }
  elapsed = micros() - startTime;
  Serial.print("32 iterations of recording & playback test took: ");
  Serial.print(elapsed);
  Serial.println(" microseconds");
  Serial.print("Average per iteration: ");
  average = elapsed / 32.0;
  Serial.print(average);
  Serial.println(" microseconds");
  if (T_record < average){
    T_record = average;
  }
  #endif

  #ifdef DISABLE_THREADS
  double period_tau_i = 10000.0;
  double tau_n=10000.0;

  double L_n =  (tau_n / period_tau_i) * T_scanKeys +
                (tau_n / period_tau_i) * T_displayUpdate + 
                (tau_n / period_tau_i) * T_decode +
                (tau_n / period_tau_i) * T_transmit +
                (tau_n / period_tau_i) * T_sig + 
                (tau_n / period_tau_i) * T_record;

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



static void GPIO_Init()
{
  // GPIO Clock Enable
  __HAL_RCC_ADC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();  

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // PA0 // JOYY
  // PA1 // JOYX

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  Serial.println("Correctly Initalised GPIO Pins");
}

static void DAC_Init()
{
  Serial.println("DAC Init");
  __HAL_RCC_DAC1_CLK_ENABLE();
  // DAC Initialization
  hdac1.Instance = DAC;


  if(HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Serial.println("DAC Init Error");
    Error_Handler();
  }


  DAC_ChannelConfTypeDef sConfig = {0};
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;

#ifdef __USING_DAC_CHANNEL_1
  if(HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Serial.println("DAC Config Error Channel 1");
    Error_Handler();
  }
#else
  // also do channel2??
  if(HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_2) != HAL_OK)
  {
    Serial.println("DAC Config Error Channel 2");
    Error_Handler();
  }
#endif


  #ifdef __USING_DAC_CHANNEL_1
  HAL_StatusTypeDef status = HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t*) dac_buffer, DAC_BUFFER_SIZE, DAC_ALIGN_8B_R);
  #else
  HAL_StatusTypeDef status = HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_2, (uint32_t*) dac_buffer, DAC_BUFFER_SIZE, DAC_ALIGN_8B_R);
  #endif

  if(status != HAL_OK)
  {
    Serial.print("BDAC attach DMA Config Error: ");
    Serial.print(status);
    Serial.print(" DAC STATE: "); Serial.println(hdac1.State);
    Serial.print("DAC DMA Handle ptr: "); Serial.println((uint32_t) hdac1.DMA_Handle1);
    Serial.print("DAC DMA Handle ptr manual: "); Serial.println((uint32_t) &hdma_dac1);
    Error_Handler();
  }
  else
  {
    Serial.println("GDAC attach DMA Config Success");
  }


  Serial.println("Successful DAC Init");
}

static void ADC_Init()
{
  __HAL_RCC_ADC_CLK_ENABLE();

  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;

  if(HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    while(1) Serial.println("Error Initalising ADC!!");
  }

  Serial.println("Successful ADC Init");

  // channel config will have to be on sample 
  // which is quite annoying
}

static void DMA_Init()
{
  Serial.println("Begin DMA");
#ifdef __USING_DAC_CHANNEL_1
  __HAL_RCC_DMA1_CLK_ENABLE();
#else
  __HAL_RCC_DMA2_CLK_ENABLE();
#endif

  // this isn't included in the pre generated cubeMX code ... internet recommends
#ifdef __USING_DAC_CHANNEL_1
  hdma_dac1.Instance = DMA1_Channel3;
#else
  hdma_dac1.Instance = DMA1_Channel4;  // Ensure this is the correct channel for DAC1
#endif
  hdma_dac1.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_dac1.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_dac1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_dac1.Init.PeriphDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_dac1.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE; 
  hdma_dac1.Init.Mode = DMA_CIRCULAR;
  hdma_dac1.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_dac1.Init.Request = DMA_REQUEST_6;

  if (HAL_DMA_Init(&hdma_dac1) != HAL_OK)
  {
    Serial.println("DMA Init Error");
    Error_Handler();
  }

  // Enable DMA transfer complete and half-transfer interrupts
  // __HAL_DMA_ENABLE_IT(&hdma_dac1, DMA_IT_TC | DMA_IT_HT | DMA_IT_TE);
  __HAL_DMA_ENABLE_IT(&hdma_dac1, DMA_IT_HT);
  __HAL_DMA_ENABLE_IT(&hdma_dac1, DMA_IT_TC);
  __HAL_DMA_ENABLE_IT(&hdma_dac1, DMA_IT_TE);

#ifdef __USING_DAC_CHANNEL_1
  __HAL_LINKDMA(&hdac1, DMA_Handle1, hdma_dac1);

  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

#else
  __HAL_LINKDMA(&hdac1, DMA_Handle2, hdma_dac1);

  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
#endif
  
  Serial.println("DMA Init Success");
}


static void TIM6_Init()
{  
#ifdef __USING_HARDWARETIMER

  sampleTimer.setup(TIM6);
  sampleTimer.setOverflow(F_SAMPLE_TIMER, HERTZ_FORMAT);
  sampleTimer.pause();

  TIM_HandleTypeDef *htim= sampleTimer.getHandle();

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(htim, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  // HAL_TIM_Base_Start_IT(htim);
  sampleTimer.resume();

  Serial.print(sampleTimer.isRunning() ? "Timer is running " : "Timer is not running "); Serial.println(sampleTimer.getTimerClkFreq());
  Serial.println(sampleTimer.hasInterrupt() ? "Timer has interrupt" : "Timer does not have interrupt");

#else
  __HAL_RCC_TIM6_CLK_ENABLE();

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 80-1;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  htim6.Init.Period = 10000-1;
  htim6.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

  if(HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Serial.println("TIM6 Init error");
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // critical for dma
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  Set_TIMx_Frequency(&htim6, F_SAMPLE_TIMER);

  
#endif
}

extern "C" void DMA1_Channel3_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_dac1);
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac)
{
  if(hdac->Instance == DAC1)
  {
    writeBuffer1 = false;
    xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
  }
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hdac)
{
  if(hdac->Instance == DAC1)
  {
    writeBuffer1 = true;
    xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
  }
}