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
    uint8_t RX_Message[8];
    static std::pair<uint8_t,uint8_t> incoming;
    
    while (1){
        xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);//Gets current message from queue

        #ifdef GET_MASS_DECODE
        monitorStackSize();
        #endif

        //Also now realise this is basc just the notesPlayed lol
        //May also want to add logic to prevent repeat keys (if this becomes an issue)
        incoming = std::make_pair(RX_Message[1],RX_Message[2]);
        
        xSemaphoreTake(voices.mutex,portMAX_DELAY);
        if (RX_Message[0] == 'P'){
            voices.voices_array[incoming.first * 12 + incoming.second].phaseAcc = 0;
            voices.notes.push_back(incoming);
        }
        else if (RX_Message[0] == 'R'){
            auto it = std::find(voices.notes.begin(),voices.notes.end(),incoming);
            if (it != voices.notes.end()){
                voices.notes.erase(it);
            }
            // for (int i = 0; i < voices.notes.size(); i++){
            //     if (voices.notes[i] == incoming) {
            //         voices.notes[i] = voices.notes.back();
            //         voices.notes.pop_back();
            //         break;
            //     }
            // }
        }
        xSemaphoreGive(voices.mutex);

        // Atomic operations:
        
        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        for (int i = 0; i < 8; i++) sysState.RX_Message[i] = RX_Message[i];//Saves message for printing
        if (sysState.slave) {//Handles slave muting
            if (RX_Message[3] == 0xFF) sysState.mute = true;
            else {
                sysState.mute = false;
                sysState.Volume = RX_Message[3];
            }
        }
        xSemaphoreGive(sysState.mutex);
    }
}


void transmitTask (void * pvParameters) {//Transmits message across CAN bus
    uint8_t msgOut[8];
    while (1) {
        xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
        #ifdef GET_MASS_TRANSMIT
        monitorStackSize();
        #endif
        xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);
        CAN_TX(0x123, msgOut);
    }
}
