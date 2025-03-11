// #include <Arduino.h>
#include <globals.h>
#include <U8g2lib.h>
#include <bitset>
#include <STM32FreeRTOS.h>
#include <scanKeysTask.h>
#include <displayUpdateTask.h>
#include <sig_gen.h>
#include <CANTask.h>
#include <ISR.h>
#include <ES_CAN.h>
/*
I'm pretty sure now the only thing left to do is ensure the DMA works
(the DAC itself works and outputs)
And from there, if the Conv(Half)CpltCallback s don't work, link sampleISR manually.
*/

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
  // HAL_Init();

  // Initialise SysClock
  // SystemClock_Config();

  //Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");


  // Assert wave_buffer as 0s initially
  // memset((uint32_t*) dac_buffer, 0UL, DAC_BUFFER_SIZE);


  //Initialise GPIO
  GPIO_Init();

  //Initialise DMA
  DMA_Init();

  //Initialise DAC
  DAC_Init();

  //Initialise timer
  TIM6_Init();

  for(int i = 0; i < DAC_BUFFER_SIZE; i++)
  {
    dac_buffer[i] = (uint8_t) (128 + (128/4)*sin(2*M_PI*i / DAC_BUFFER_SIZE));
  }

  HAL_Delay(1);

	// HAL_TIM_Base_Start_IT(&htim6);
  HAL_TIM_Base_Start(&htim6);


  // for(int i = 0; i < 10*DAC_BUFFER_SIZE; i++)
  // {
  //   Serial.println(hdma_dac1.Instance->CNDTR);
  // }

  //Initialise Queue
  msgInQ = xQueueCreate(36,8);
  msgOutQ = xQueueCreate(36,8);

  //Initialise Mutex
  sysState.mutex = xSemaphoreCreateMutex();

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
  // Serial.println("CAN bus started");

  //Initialise threads
  TaskHandle_t scanKeysHandle = NULL;
  xTaskCreate(
    scanKeysTask,		/* Function that implements the task */
    "scanKeys",		/* Text name for the task */
    128,      		/* Stack size in words, not bytes */
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

  //Start threads
  vTaskStartScheduler();
}

void loop() {

}



static void GPIO_Init()
{
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
    // Serial.print("DMA Flag: "); Serial.println(__HAL_DMA_GET_FLAG(&hdma_dac1, DMA_FLAG_TE1));
    // Serial.print("DAC Error Code: "); Serial.println(hdac1.ErrorCode);
  }


  Serial.println("Successful DAC Init");
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
  __HAL_DMA_ENABLE_IT(&hdma_dac1, DMA_IT_HT);

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

/*
TODO: CHANGE TO A FIXED FREQUENCY!! F_SAMPLE_TIMER
*/
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

  // sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  // if (HAL_TIM_ConfigClockSource(&htim6, &sClockSourceConfig) != HAL_OK)
  // {
  //   Error_Handler();
  // }
  // if (HAL_TIM_OC_Init(&htim6) != HAL_OK)
  // {
  //   Error_Handler();
  // }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // critical for dma
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  // sConfigOC.OCMode = TIM_OCMODE_TIMING;
  // sConfigOC.Pulse = 0;
  // sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  // sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  // if (HAL_TIM_OC_ConfigChannel(&htim6, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  // {
  //   Error_Handler();
  // }

  // Set_TIMx_Frequency(&htim6, 1000);

  // __HAL_TIM_CLEAR_FLAG(&htim6, TIM_IT_UPDATE);
  // __HAL_TIM_ENABLE_IT(&htim6, TIM_IT_UPDATE);

  // HAL_NVIC_SetPriority(TIM6_IRQn, 0, 0);
  // HAL_NVIC_EnableIRQ(TIM6_IRQn);

  // HAL_Delay(1000);
  // if(HAL_TIM_Base_Start(&htim6) != HAL_OK)
  // {
  //   Serial.println("NO FUCKING IT START TIM6");
  // }

  // HAL_TIM_Base_Stop(&htim6);

  // HAL_NVIC_SetPriority(TIM6_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 2, 0);
  // HAL_NVIC_EnableIRQ(TIM6_IRQn);

#endif
}

// extern "C" void DMA1_Channel3_IRQHandler(void) {
//   HAL_DMA_IRQHandler(&hdma_dac1);
// }

// extern "C" void TIM6_IRQHandler(void)
// {
//   HAL_TIM_IRQHandler(&htim6);
// }

// __weak void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac);
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac)
{
  if(hdac->Instance == DAC1)
  {
    writeBuffer1 = false;
    // Serial.println("ConvCOMPLETE");
    xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
  }
}
// __weak void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hdac);
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hdac)
{
  if(hdac->Instance == DAC1)
  {
    writeBuffer1 = true;
    // Serial.println("ConvHALF");
    xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
  }
}
