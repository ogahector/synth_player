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
    bool doomMode = false;
    bool joystickPress = false;
    } sysState_t;