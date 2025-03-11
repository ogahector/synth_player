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

void decodeTask(void * pvParameters){
    uint8_t RX_Message[8];
    int index;
    
    while (1){
        xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);//Gets current message from queue
        xSemaphoreTake(notesBuffer.mutex, portMAX_DELAY);
        index = RX_Message[2];
        if (RX_Message[0] == 'R')
        {
          // Remove first encountered instance of note from notesBuffer.notesPlayed
          for(size_t i = 0; i < notesBuffer.notesPlayed[index].size(); i++)
          {
            if(notesBuffer.notesPlayed[index][i] == RX_Message[1])
            {
              notesBuffer.notesPlayed[index].erase(notesBuffer.notesPlayed[index].begin() + i);
              break;
            }
          }
        }
        else if (RX_Message[0] == 'P')
        {
          notesBuffer.notesPlayed[index].push_back(RX_Message[1]);
        }
        
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
        xSemaphoreGive(notesBuffer.mutex);
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