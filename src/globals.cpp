//Global variables cpp
#include <STM32FreeRTOS.h>
#include <U8g2lib.h>
#include <globals.h>


// DAC Related
std::atomic_bool writeBuffer1 = false;

volatile uint8_t dac_buffer[DAC_BUFFER_SIZE];
volatile uint8_t* dac_write_HEAD = &dac_buffer[HALF_DAC_BUFFER_SIZE];

//Step Sizes
constexpr uint32_t hz2stepSize(float freq)
{
  return (uint32_t) ((4294967296 * freq) / F_SAMPLE_TIMER);
}
const uint32_t stepSizes[] = {
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

const int baseFreqs[] = {
  242,
  264,
  286,
  308,
  330,
  352,
  374,
  396,
  418,
  440,
  462,
  484
};

//SysState
sysState_t sysState;

//Record
record_t record;

//Voices
voices_t voices;

//CAN Queues
QueueHandle_t msgInQ;
QueueHandle_t msgOutQ;

// Semaphores
SemaphoreHandle_t CAN_TX_Semaphore;
SemaphoreHandle_t signalBufferSemaphore;

//Display driver object
U8G2_SSD1305_128X32_ADAFRUIT_F_HW_I2C u8g2(U8G2_R0);

//Hardware Timer
HardwareTimer sampleTimer;

// DAC
DAC_HandleTypeDef hdac1;

// DMA
DMA_HandleTypeDef hdma_dac1;

// TIM6
TIM_HandleTypeDef htim6;

#ifdef STACK_SIZE_MONITORING
void monitorStackSize()
{
  static uint32_t prevTime = micros();

  if(micros() - prevTime > 1000000) // every second
  {
    prevTime = micros();
    // Passing in NULL is for the current task!
    // I was worried it might be for the stack but it's fine dw
    UBaseType_t unusedStackSize = uxTaskGetStackHighWaterMark(NULL);
    const char* taskName = pcTaskGetName(NULL);
    
    Serial.print(taskName);
    Serial.print(" Current Unused Stack Size: ");
    Serial.print(unusedStackSize);
    Serial.print(" (words) | ");
    Serial.print(unusedStackSize * 4);
    Serial.println(" (bytes)");
  }
}
#endif