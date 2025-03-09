#pragma once

//Global header file
#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <U8g2lib.h>
#include <bitset>

#define _RECEIVER // BY DEFAULT, THE DEVICE IS A RECEIVER
// UNCOMMENT THE ABOVE LINE TO MAKE THE DEVICE A SENDER
#ifndef _RECEIVER
#define _SENDER
#endif

// Multi Note Constants
#define MAX_POLYPHONY 12// Max number of simultaneous notes
extern volatile uint32_t activeStepSizes[12];//Has one for each key

extern const uint32_t stepSizes[];

typedef struct __sysState_t{
    std::bitset<32> inputs;
    int Volume;
    bool mute = false;
    bool slave = false;
    SemaphoreHandle_t mutex;
    uint8_t RX_Message[8];   
    int Octave = 0;
    // 0: Home Screen, with volume and octave control
    // 1: Menu
    // 2: DOOM
    // 3: Wave
    std::bitset<4> activityList; // Shows the current state of the system. If a bit is set, then the system is currently in that state.
    bool joystickPress = false;
    int joystickDirection = 0;
    } sysState_t;

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
#define OUTL_PIN A4
#define OUTR_PIN A3
//analoge input
#define JOYY_PIN A0
#define JOYX_PIN A1
//multiplexer bit
#define DEN_BIT 3
#define DRST_BIT 4
#define HKOW_BIT 5
#define HKOE_BIT 6