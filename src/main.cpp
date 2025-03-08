// #include <Arduino.h>
#include <U8g2lib.h>
#include <bitset>
#include <STM32FreeRTOS.h>
#include <globals.h>
#include <scanKeysTask.h>
#include <displayUpdateTask.h>
#include <sig_gen.h>
#include <CANTask.h>
#include <ISR.h>
#include <ES_CAN.h>

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

void TIM2_Init();
void DAC_Init();
void DMA_Init();
void GPIO_Init();

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

  // Assert wave_buffer as 0s initially
  memset((uint8_t*) dac_buffer1, 0, DAC_BUFFER_SIZE);
  memset((uint8_t*) dac_buffer2, 0, DAC_BUFFER_SIZE);

  // Assert TIM2 OFF
  // sampleTimer.pause();
  // sampleTimer.timerHandleDeinit();

  // Initialise HAL
  HAL_Init();

  //Initialise GPIO
  GPIO_Init();

  //Initialise DMA
  DMA_Init();

  //Initialise DAC
  DAC_Init();

  //Initialise timer
  TIM2_Init();
  Serial.println(sampleTimer.isRunning() ? "Timer is running" : "Timer is not running");

  //Initialise Queue
  msgInQ = xQueueCreate(36,8);
  msgOutQ = xQueueCreate(36,8);

  //Initialise Mutex
  sysState.mutex = xSemaphoreCreateMutex();
  // Serial.println("Mutex created");

  //Initialise CAN Semaphore
  CAN_TX_Semaphore = xSemaphoreCreateCounting(3,3);
  // Serial.println("CAN TX Semaphore created");

  //Initialise signal semaphore
  signalBufferSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(signalBufferSemaphore);
  // Serial.println("Signal Buffer Semaphore created");

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
  // Serial.println("scanKeysTask created");

  TaskHandle_t displayUpdateHandle = NULL;
  xTaskCreate(
    displayUpdateTask,		/* Function that implements the task */
    "displayUpdate",		/* Text name for the task */
    256,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    1,			/* Task priority */
    &displayUpdateHandle /* Pointer to store the task handle */
  );
  // Serial.println("displayUpdateTask created");
  
  TaskHandle_t decodeHandle = NULL;
  xTaskCreate(
    decodeTask,		/* Function that implements the task */
    "decode",		/* Text name for the task */
    64,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    3,			/* Task priority */
    &decodeHandle /* Pointer to store the task handle */
  );
  // Serial.println("decodeTask created");

  TaskHandle_t transmitHandle = NULL;
  xTaskCreate(
    transmitTask,		/* Function that implements the task */
    "transmit",		/* Text name for the task */
    64,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    3,			/* Task priority */
    &transmitHandle /* Pointer to store the task handle */
  );
  // Serial.println("transmitTask created");

  TaskHandle_t signalHandle = NULL;
  xTaskCreate(
    signalGenTask,		/* Function that implements the task */
    "signal",		/* Text name for the task */
    256,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    1,			/* Task priority */
    &signalHandle /* Pointer to store the task handle */
  );
  // Serial.println("signalGenTask created");

  //Start threads
  vTaskStartScheduler();
}

void loop() {

}



void GPIO_Init()
{
  // GPIO Initialization
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // GPIO Clock Enable
  __HAL_RCC_GPIOA_CLK_ENABLE();

  // GPIO Analog Out Configuration
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  // GPIO_InitStruct.Pin = OUTR_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void DAC_Init()
{
  Serial.println("DAC Init");
  // DAC Initialization
  hdac.Instance = DAC;

  // Reset DAC to a known state
  if(HAL_DAC_DeInit(&hdac) != HAL_OK)
  {
    Serial.println("DAC DeInit Error ?????");
    Error_Handler();
  }

  if(HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Serial.println("DAC Init Error");
    Error_Handler();
  }


  DAC_ChannelConfTypeDef sConfig = {0};
  sConfig.DAC_Trigger = DAC_TRIGGER_T2_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if(HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Serial.println("DAC Config Error");
    Error_Handler();
  }

  // Initialise DMA
  // see about casting to uint32_t* bc i had some trouble in instrumentation
  HAL_StatusTypeDef status = HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*) dac_read_buffer, DAC_BUFFER_SIZE, DAC_ALIGN_8B_R);

  if(status != HAL_OK)
  {
    Serial.print("DAC attach DMA Config Error: ");
    Serial.print(status);
    Serial.print(" DAC STATE: "); Serial.println(hdac.State);
    Error_Handler();
  }
  else
  {
    Serial.println("DAC attach DMA Config Success");
  }
  
}

void DMA_Init()
{
  __HAL_RCC_DMA1_CLK_ENABLE();

  hdma_dac.Instance = DMA1_Channel3;  // Ensure this is the correct channel for DAC1
  hdma_dac.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_dac.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_dac.Init.MemInc = DMA_MINC_ENABLE;
  hdma_dac.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_dac.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE; 
  hdma_dac.Init.Mode = DMA_CIRCULAR;  // Enable circular mode
  hdma_dac.Init.Priority = DMA_PRIORITY_HIGH;

  if (HAL_DMA_Init(&hdma_dac) != HAL_OK)
  {
    Serial.println("DMA Init Error");
    Error_Handler();
  }
  
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

  __HAL_LINKDMA(&hdac, DMA_Handle1, hdma_dac);

  Serial.println("DMA Init Success");
}

void TIM2_Init()
{  
  sampleTimer.setup(TIM2);
  sampleTimer.pause();

  TIM_HandleTypeDef *htim = sampleTimer.getHandle();

  sampleTimer.setOverflow(F_SAMPLE_TIMER, HERTZ_FORMAT);
  sampleTimer.setMode(1, TIMER_OUTPUT_COMPARE);

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // critical for dma
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(htim, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }


  sampleTimer.attachInterrupt(sampleISR);

  sampleTimer.resume();

  Serial.println(sampleTimer.isRunning() ? "Timer is running" : "Timer is not running");
  Serial.println(sampleTimer.hasInterrupt() ? "Timer has interrupt" : "Timer does not have interrupt");
}

