//CAN TX and RX Tasks
#include <globals.h>
#include <ES_CAN.h>
#include <CANTask.h>
#include <limits>


template <typename T> // no need to export tbh
void saturate(volatile T& val, T min, T max){
    if (val < min) val = min;
    else if (val > max) val = max;
}

void decodeTask(void * pvParameters){
    uint32_t activeStepSizesLocal[12];
    uint8_t RX_Message[8];
    
    while (1){
        xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);//Gets current message from queue
        //__atomic_load(&activeStepSizes, &activeStepSizesLocal, __ATOMIC_RELAXED);
        volatile uint32_t* keyPressIndex = &activeStepSizes[RX_Message[2]]; // will make it more readable
        // we fix the noise floor error by ensuring no overflow occurs
        if (RX_Message[0] == 'R'){//Key is reset if released
            activeStepSizes[RX_Message[2]] = 0;
            if (RX_Message[1] - 4 >= 0)
            {
                activeStepSizes[RX_Message[2]] -= stepSizes[RX_Message[2]] << (RX_Message[1] - 4);
                // if (*keyPressIndex > *keyPressIndex << (RX_Message[1] - 4)) 
                // {
                //     *keyPressIndex -= *keyPressIndex << (RX_Message[1] - 4);
                // } 
                // else 
                // {
                //     *keyPressIndex = 0;
                // }
            }
            else
            {
                activeStepSizes[RX_Message[2]] += stepSizes[RX_Message[2]] >> abs(RX_Message[1] - 4);
                // if (*keyPressIndex < *keyPressIndex >> abs(RX_Message[1] - 4)) 
                // {
                //     *keyPressIndex += *keyPressIndex >> abs(RX_Message[1] - 4);
                // } 
                // else 
                // {
                //     *keyPressIndex = 0;
                // }
            }
        }
        else if (RX_Message[0] == 'P'){//Key is shifted if pressed
            if (RX_Message[1] - 4 >= 0) 
            {
                activeStepSizes[RX_Message[2]] += stepSizes[RX_Message[2]] << (RX_Message[1] - 4);
                // if (UINT32_MAX - *keyPressIndex > *keyPressIndex << (RX_Message[1] - 4)) 
                // {
                //     *keyPressIndex += *keyPressIndex << (RX_Message[1] - 4);
                // } 
                // else 
                // {
                //     *keyPressIndex = UINT32_MAX;
                // }
            }
            else
            {
                activeStepSizes[RX_Message[2]] -= stepSizes[RX_Message[2]] >> abs(RX_Message[1] - 4);
                // if (UINT32_MAX - *keyPressIndex > *keyPressIndex >> abs(RX_Message[1] - 4)) 
                // {
                //     *keyPressIndex -= *keyPressIndex >> abs(RX_Message[1] - 4);
                // } 
                // else 
                // {
                //     *keyPressIndex = 0;
                // }
            } 
        }

        //__atomic_store_n(&activeStepSizes, &activeStepSizesLocal, __ATOMIC_RELAXED);
        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        if (sysState.slave) {//Handles slave muting
            if (RX_Message[3] == 0xFF) sysState.mute = true;
            else {
                sysState.mute = false;
                sysState.Volume = RX_Message[3];
            }
        }
        for (int i = 0; i < 8; i++) sysState.RX_Message[i] = RX_Message[i];//Saves message for printing
        xSemaphoreGive(sysState.mutex);
    }
}

void transmitTask (void * pvParameters) {//Transmits message across CAN bus
    uint8_t msgOut[8];
    while (1) {
        xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
        xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);
        CAN_TX(0x123, msgOut);
    }
}