//Global variables cpp
#include <STM32FreeRTOS.h>
#include <U8g2lib.h>
#include <globals.h>


// Multi Note Constants
volatile uint32_t activeStepSizes[12] = {0, 0, 0, 0, 0, 0, 
                                         0, 0, 0, 0, 0, 0};//Has one for each key

//Step Sizes
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

//SysState
sysState_t sysState;

//CAN Queues
QueueHandle_t msgInQ;
QueueHandle_t msgOutQ;
SemaphoreHandle_t CAN_TX_Semaphore;

//Display driver object
U8G2_SSD1305_128X32_ADAFRUIT_F_HW_I2C u8g2(U8G2_R0);

//Hardware Timer
HardwareTimer sampleTimer(TIM1);