// #include <Arduino.h>
// #include <Inc/stm32l4xx_hal.h>
// #include <stm32l4xx_it.h>
// #include <stm32l4xx_hal_conf.h>
// #include <stm32l4xx_hal.h>
#include <globals.h>
#include <U8g2lib.h>
#include <bitset>
#include <STM32FreeRTOS.h>

#include <sig_gen.h>
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


static void TIM2_Init();
static void DAC_Init();
static void DMA_Init();
static void GPIO_Init();


void setup() {
  // put your setup code here, to run once:
  //Set pin directions
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
  pinMode(JOYX_PIN, INPUT);
  pinMode(JOYY_PIN, INPUT);

  //Initialise display
  setOutMuxBit(DRST_BIT, LOW);  //Assert display logic reset
  delayMicroseconds(2);
  setOutMuxBit(DRST_BIT, HIGH);  //Release display logic reset
  u8g2.begin();
  setOutMuxBit(DEN_BIT, HIGH);  //Enable display power supply

  // Initialise HAL
  HAL_Init();

  // Initialise SysClock
  SystemClock_Config();

  //Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");

  // Assert wave_buffer as 0s initially
  memset((uint32_t*) dac_buffer, 0UL, DAC_BUFFER_SIZE);
  for(int i = 0; i < DAC_BUFFER_SIZE; i++)
  {
    dac_buffer[i] = 2048 + 2048*sin(2*M_PI*i / DAC_BUFFER_SIZE);
  }

  //Initialise GPIO
  GPIO_Init();

  //Initialise DMA
  DMA_Init();

  //Initialise DAC
  DAC_Init();

  //Initialise timer
  TIM2_Init();
  HAL_Delay(10);
  Serial.println(__HAL_TIM_GET_COUNTER(&htim2) ? "Timer is running" : "Timer is not running");
  Serial.println(__HAL_RCC_DMA1_IS_CLK_ENABLED() ? "DMA Timer is running" : "DMA Timer is not running");

#ifdef __USING_DAC_CHANNEL_1
  HAL_StatusTypeDef status = HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t*) dac_buffer, DAC_BUFFER_SIZE, DAC_ALIGN_12B_R);
#else
  HAL_StatusTypeDef status = HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_2, (uint32_t*) dac_buffer, DAC_BUFFER_SIZE, DAC_ALIGN_8B_R);
#endif

  if(status != HAL_OK)
  {
    Serial.print("DAC attach DMA Config Error: ");
    Serial.print(status);
    Serial.print(" DAC STATE: "); Serial.println(hdac1.State);
    Serial.print("DAC DMA Handle ptr: "); Serial.println((uint32_t) hdac1.DMA_Handle1);
    Serial.print("DAC DMA Handle ptr manual: "); Serial.println((uint32_t) &hdma_dac1);
    Error_Handler();
  }
  else
  {
    Serial.println("DAC attach DMA Config Success");
  }
  Serial.print("DMA Flag: "); Serial.println(__HAL_DMA_GET_FLAG(&hdma_dac1, DMA_FLAG_TE1));
  Serial.print("DAC Error Code: "); Serial.println(hdac1.ErrorCode);

  //Initialise Queue
  msgInQ = xQueueCreate(36,8);
  msgOutQ = xQueueCreate(36,8);

  //Initialise Mutex
  sysState.mutex = xSemaphoreCreateMutex();

  sysState.activityList = HOME;

  sysState.activityList = HOME;

  //Initialise CAN Semaphore
  CAN_TX_Semaphore = xSemaphoreCreateCounting(3,3);

  //Initialise signal semaphore
  signalBufferSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(signalBufferSemaphore);

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
    4,			/* Task priority */
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
    3,			/* Task priority */
    &decodeHandle /* Pointer to store the task handle */
  );

  TaskHandle_t transmitHandle = NULL;
  xTaskCreate(
    transmitTask,		/* Function that implements the task */
    "transmit",		/* Text name for the task */
    64,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    3,			/* Task priority */
    &transmitHandle /* Pointer to store the task handle */
  );

  TaskHandle_t signalHandle = NULL;
  xTaskCreate(
    signalGenTask,		/* Function that implements the task */
    "signal",		/* Text name for the task */
    256,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    1,			/* Task priority */
    &signalHandle /* Pointer to store the task handle */
  );

  Serial.println("Setup complete, starting scheduler...");
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



static void GPIO_Init()
{
  // GPIO Initialization
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // GPIO Clock Enable
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

static void DAC_Init()
{
  Serial.println("DAC Init");
  // DAC Initialization
  hdac1.Instance = DAC;


  if(HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Serial.println("DAC Init Error");
    Error_Handler();
  }


  DAC_ChannelConfTypeDef sConfig = {0};
  sConfig.DAC_Trigger = DAC_TRIGGER_T2_TRGO;
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_ENABLE;
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
  
}

static void DMA_Init()
{
  Serial.println("Entering DMA Init");
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
  hdma_dac1.Init.PeriphDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_dac1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD; 
  hdma_dac1.Init.Mode = DMA_CIRCULAR;
  hdma_dac1.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_dac1.Init.Request = DMA_REQUEST_6;

  if (HAL_DMA_Init(&hdma_dac1) != HAL_OK)
  {
    Serial.println("DMA Init Error");
    Error_Handler();
  }

  // Enable DMA transfer complete and half-transfer interrupts
  __HAL_DMA_ENABLE_IT(&hdma_dac1, DMA_IT_TC | DMA_IT_HT | DMA_IT_TE);

#ifdef __USING_DAC_CHANNEL_1
  __HAL_LINKDMA(&hdac1, DMA_Handle1, hdma_dac1);

  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
#else
  __HAL_LINKDMA(&hdac1, DMA_Handle2, hdma_dac1);

  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
#endif
  
  Serial.println("DMA Init Success");
}

/*
TODO: CHANGE TO A FIXED FREQUENCY!! F_SAMPLE_TIMER
*/
static void TIM2_Init()
{  
#ifdef __USING_HARDWARETIMER

  sampleTimer.setup(TIM2);
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

  HAL_TIM_Base_Start_IT(htim);

  Serial.print(sampleTimer.isRunning() ? "Timer is running " : "Timer is not running "); Serial.println(sampleTimer.getTimerClkFreq());
  Serial.println(sampleTimer.hasInterrupt() ? "Timer has interrupt" : "Timer does not have interrupt");

#else
  __HAL_RCC_TIM2_CLK_ENABLE();

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  htim2.Init.Period = 79999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

  if(HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Serial.println("TIM2 Init error");
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // critical for dma
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  // sampleTimer.setOverflow(F_SAMPLE_TIMER, HERTZ_FORMAT);
  // sampleTimer.setMode(1, TIMER_OUTPUT_COMPARE);


  if(HAL_TIM_Base_Start(&htim2) != HAL_OK)
  {
    Serial.println("NO FUCKING IT START TIM6");
  } 

#endif
  Serial.print("DMA State: "); Serial.println(hdma_dac1.State);
}

// __weak void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hadc);
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hadc)
{
  writeBuffer1 = false;
  Serial.println("ConvCOMPLETE");
  xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
}
// __weak void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hadc);
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hadc)
{
  writeBuffer1 = true;
  Serial.println("ConvHALF");
  xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
}



static void GPIO_Init()
{
  // GPIO Initialization
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // GPIO Clock Enable
  __HAL_RCC_GPIOA_CLK_ENABLE();

}

static void DAC_Init()
{
  Serial.println("DAC Init");
  // DAC Initialization
  hdac1.Instance = DAC;


  if(HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Serial.println("DAC Init Error");
    Error_Handler();
  }


  DAC_ChannelConfTypeDef sConfig = {0};
  sConfig.DAC_Trigger = DAC_TRIGGER_T2_TRGO;
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_ENABLE;
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
  
}

static void DMA_Init()
{
  Serial.println("Entering DMA Init");
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
  hdma_dac1.Init.PeriphDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_dac1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD; 
  hdma_dac1.Init.Mode = DMA_CIRCULAR;
  hdma_dac1.Init.Priority = DMA_PRIORITY_HIGH;
  hdma_dac1.Init.Request = DMA_REQUEST_6;

  if (HAL_DMA_Init(&hdma_dac1) != HAL_OK)
  {
    Serial.println("DMA Init Error");
    Error_Handler();
  }

  // Enable DMA transfer complete and half-transfer interrupts
  __HAL_DMA_ENABLE_IT(&hdma_dac1, DMA_IT_TC | DMA_IT_HT | DMA_IT_TE);

#ifdef __USING_DAC_CHANNEL_1
  __HAL_LINKDMA(&hdac1, DMA_Handle1, hdma_dac1);

  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
#else
  __HAL_LINKDMA(&hdac1, DMA_Handle2, hdma_dac1);

  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
#endif
  
  Serial.println("DMA Init Success");
}

/*
TODO: CHANGE TO A FIXED FREQUENCY!! F_SAMPLE_TIMER
*/
static void TIM2_Init()
{  
#ifdef __USING_HARDWARETIMER

  sampleTimer.setup(TIM2);
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

  HAL_TIM_Base_Start_IT(htim);

  Serial.print(sampleTimer.isRunning() ? "Timer is running " : "Timer is not running "); Serial.println(sampleTimer.getTimerClkFreq());
  Serial.println(sampleTimer.hasInterrupt() ? "Timer has interrupt" : "Timer does not have interrupt");

#else
  __HAL_RCC_TIM2_CLK_ENABLE();

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  htim2.Init.Period = 79999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

  if(HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Serial.println("TIM2 Init error");
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // critical for dma
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  // sampleTimer.setOverflow(F_SAMPLE_TIMER, HERTZ_FORMAT);
  // sampleTimer.setMode(1, TIMER_OUTPUT_COMPARE);


  if(HAL_TIM_Base_Start(&htim2) != HAL_OK)
  {
    Serial.println("NO FUCKING IT START TIM6");
  } 

#endif
  Serial.print("DMA State: "); Serial.println(hdma_dac1.State);
}

// __weak void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hadc);
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hadc)
{
  writeBuffer1 = false;
  Serial.println("ConvCOMPLETE");
  xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
}
// __weak void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hadc);
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hadc)
{
  writeBuffer1 = true;
  Serial.println("ConvHALF");
  xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
}