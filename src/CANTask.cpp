//CAN TX and RX Tasks
#include <globals.h>
#include <ES_CAN.h>
#include <CANTask.h>
#include <scanKeysTask.h>
#include <limits>


template <typename T> // no need to export tbh
void saturate(volatile T& val, T min, T max){
    if (val < min) val = min;
    else if (val > max) val = max;
}


//NEED TO CHANGE LOGIC HERE TO USE VOCIES
void decodeTask(void * pvParameters){
    uint32_t activeStepSizesLocal[12];
    uint8_t RX_Message[8];
    static uint8_t voicesIndex = 0;
    
    while (1){
        xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);//Gets current message from queue
        xSemaphoreTake(voices.mutex,portMAX_DELAY);
        if (RX_Message[0] == 'P'){
            if (voicesIndex < MAX_VOICES-1){
                if (voices.voices_array[voicesIndex+1].active == 1){
                    Serial.println("Voice already active (full?)");
                }
                else{
                    voices.voices_array[voicesIndex+1].phaseAcc = 0;
                    if (RX_Message[1] - 4 > 0){
                        voices.voices_array[voicesIndex+1].phaseInc = stepSizes[RX_Message[2]] << (RX_Message[1]-4);
                    }
                    else{
                        voices.voices_array[voicesIndex+1].phaseInc = stepSizes[RX_Message[2]] >> abs(RX_Message[1] - 4);
                    }
                    voices.voices_array[voicesIndex+1].active = 1;
                    voices.voices_array[voicesIndex+1].volume = 8 - voicesIndex;
                    voicesIndex++;
                    if (voicesIndex >= MAX_VOICES) voicesIndex = MAX_VOICES - 1;
                } 
            }

        }
        else if (RX_Message[0] == 'R'){
            if (voices.voices_array[voicesIndex].active == 0){
                Serial.println("Voice already inactive (empty?)");
            }
            else{
                voices.voices_array[voicesIndex].active = 0;
                voices.voices_array[voicesIndex].phaseAcc = 0;
                voices.voices_array[voicesIndex].phaseInc = 0;
                voices.voices_array[voicesIndex].volume = 0;
                voicesIndex--;
                if (voicesIndex < 0) voicesIndex = 0;
            }
        }
        xSemaphoreGive(voices.mutex);
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