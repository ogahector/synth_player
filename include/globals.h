#pragma once

//Global header file
#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <sysState_def.h>
#include <U8g2lib.h>


// Multi Note Constants
#define MAX_POLYPHONY 12// Max number of simultaneous notes
extern volatile uint32_t activeStepSizes[12];//Has one for each key

extern const uint32_t stepSizes[];

//SysState
extern sysState_t sysState;

//CAN Queues
extern QueueHandle_t msgInQ;
extern QueueHandle_t msgOutQ;
extern SemaphoreHandle_t CAN_TX_Semaphore;

//Display driver object
extern U8G2_SSD1305_128X32_ADAFRUIT_F_HW_I2C u8g2;

//Hardware Timer
extern HardwareTimer sampleTimer;

//Pin definitions
//Row select and enable
#define RA0_PIN  D3
#define RA1_PIN  D6
#define RA2_PIN D12
#define REN_PIN  A5
//input and output
#define C0_PIN  A2
#define C1_PIN  D9
#define C2_PIN  A6
#define C3_PIN  D1
#define OUT_PIN D11
//analogue out
#define OUTL_PIN  A4
#define OUTR_PIN  A3
//analoge input
#define JOYY_PIN  A0
#define JOYX_PIN  A1
//multiplexer bit
#define DEN_BIT 3
#define DRST_BIT 4
#define HKOW_BIT 5
#define HKOE_BIT 6