  #pragma once
  #include <bitset>
  #include <STM32FreeRTOS.h>
  
  typedef struct __sysState_t{
    std::bitset<32> inputs;
    int Volume;
    bool mute = false;
    bool slave = false;
    SemaphoreHandle_t mutex;
    uint8_t RX_Message[8];   
    int Octave = 0;
    std::bitset<4> activityList; // Shows the current state of the system. If a bit is set, then the system is currently in that state.
    // 0: Home Screen, with volume and octave control
    // 1: Menu
    // 2: DOOM
    // 3: Wave
    bool joystickPress = false;
    int joystickDirection = 0;
    } sysState_t;