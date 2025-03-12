#pragma once

//Global header file
// #include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <U8g2lib.h>
#include <bitset>
#include <map>
#include <vector>
#include <doom_def.h>

#define F_SAMPLE_TIMER 20000 // Hz


#define DAC_BUFFER_SIZE 4410 // effective size will be 2x
#define HALF_DAC_BUFFER_SIZE (DAC_BUFFER_SIZE / 2)

#define NUM_WAVES 4
#define __USING_DAC_CHANNEL_1
#define MAX_VOICES 8
// #define __USING_HARDWARETIMER

extern volatile bool writeBuffer1;
extern volatile uint8_t dac_buffer[DAC_BUFFER_SIZE];
extern volatile uint8_t* dac_write_HEAD;

extern const uint32_t stepSizes[];
extern const int baseFreqs[];

typedef enum __waveform_t {
    SAWTOOTH = 0,
    SINE = 1,
    SQUARE = 2,
    TRIANGLE = 3
} waveform_t;

//SysState
typedef struct __sysState_t{
    std::bitset<32> inputs;
    int Volume;
    bool mute = false;
    bool slave = false; // SLAVE sends, MASTER receives
    SemaphoreHandle_t mutex;
    uint8_t RX_Message[8];   
    int Octave = 0;
    bool doomMode = false;
    bool joystickPress = false;
    waveform_t currentWaveform;
} sysState_t;

typedef struct __voice_t{
    uint32_t phaseAcc = 0;
    uint32_t phaseInc = 0;
    uint8_t active = 0;
    uint8_t volume = 0;
} voice_t;

typedef struct __voices_t{
    voice_t voices_array[MAX_VOICES] = {0};
    SemaphoreHandle_t mutex;
} voices_t;

extern sysState_t sysState;
extern voices_t voices;

extern std::vector< std::vector<int> > notesPlayed;

//CAN Queues
extern QueueHandle_t msgInQ;
extern QueueHandle_t msgOutQ;
extern SemaphoreHandle_t CAN_TX_Semaphore;
extern SemaphoreHandle_t signalBufferSemaphore;

//Display driver object
extern U8G2_SSD1305_128X32_ADAFRUIT_F_HW_I2C u8g2;

//Hardware Timer
extern HardwareTimer sampleTimer;

// DAC Def 
extern DAC_HandleTypeDef hdac1;

// DMA Def
extern DMA_HandleTypeDef hdma_dac1;

// TIM Deg
extern TIM_HandleTypeDef htim6;

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