#pragma once

//Global header file
// #include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <U8g2lib.h>
#include <bitset>
#include <map>
#include <vector>
#include <doom.h>

//RX_Message values:
 //0 - 'P' or 'R' for press or release
 //1 - Octave
 //2 - Key
 //3 - Volume
 //4 - Recording index [1] (in recording mode, unused in scan keys)
 //5 - Recording index [2] (in recording mode, unused in scan keys)

// Uncomment the following to disable threads and ISRs for testing one task.
//#define DISABLE_THREADS

// Uncomment to enable test mode (worst-case execution measurement) for one task
//#define TEST_SCANKEYS
//#define TEST_DISPLAYUPDATE
//#define TEST_DECODE
//#define TEST_TRANSMIT

#define LOOPBACK false

#define F_SAMPLE_TIMER 22000 // Hz
//Aiming for 1-10ms latency.

#define DAC_BUFFER_SIZE 750 // Should ideally be freq * desired latency
#define HALF_DAC_BUFFER_SIZE (DAC_BUFFER_SIZE / 2)

#define NUM_WAVES 4
#define __USING_DAC_CHANNEL_1


#define LUT_BITS 11
#define LUT_SIZE 2048
// #define __USING_HARDWARETIMER

extern volatile bool writeBuffer1;
extern volatile uint8_t dac_buffer[DAC_BUFFER_SIZE];
extern volatile uint8_t* dac_write_HEAD;

extern const uint32_t stepSizes[];
extern const int baseFreqs[];

typedef enum __waveform_t {
    SINE = 0,
    SQUARE = 1,
    SAWTOOTH = 2,
    TRIANGLE = 3
} waveform_t;


    // 0: Home Screen, with volume and octave control
    // 1: Menu
    // 2: DOOM
    // 3: Wave
typedef enum __activityList_t{
    HOME = 0,
    MENU = 1,
    DOOM = 2,
    WAVE = 3,
    RECORDING = 4,
    } activityList_t;


typedef struct __sysState_t{
    std::bitset<32> inputs;
    int Volume = 5;
    bool mute = false;
    bool slave = false;
    SemaphoreHandle_t mutex;
    uint8_t RX_Message[8];   
    int Octave = 1;
    activityList_t activityList;
    bool joystickPress = false;
    int joystickHorizontalDirection = 0;
    int joystickVerticalDirection = 0;
    waveform_t currentWaveform;
} sysState_t;

typedef struct __record_t{
    bool recording = false;
    bool playback = false;
    uint8_t active_tracks = 0b0000; //Active tracks with 1 hot encoding.
    uint8_t current_track = 0; //Track you are currently recording / playing
    SemaphoreHandle_t mutex;//Added here since I don't want to lock the systate mutex too much (deadlock potential or sm idk?)
} record_t;

typedef struct __voice_t{
    uint32_t phaseAcc = 0;
    uint32_t phaseInc = 0;
} voice_t;

typedef struct __voices_t{
    std::vector<std::pair<uint8_t, uint8_t>> notes; // Stores octave and key/note played
    voice_t voices_array[108]; //Index by octave * 12 + key/note played
    SemaphoreHandle_t mutex;
} voices_t;

extern sysState_t sysState;
extern record_t record;
extern voices_t voices;


//CAN Queues
extern QueueHandle_t msgInQ;
extern QueueHandle_t msgOutQ;
extern SemaphoreHandle_t CAN_TX_Semaphore;
extern SemaphoreHandle_t signalBufferSemaphore;

#if !LOOPBACK
//Master internal queue
extern QueueHandle_t msgInternalQ;
#endif

//Display driver object
extern U8G2_SSD1305_128X32_ADAFRUIT_F_HW_I2C u8g2;

//Hardware Timer
extern HardwareTimer sampleTimer;

// ADC Def
extern ADC_HandleTypeDef hadc1;

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