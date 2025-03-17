#include <recordTask.h>

//RX_Message values:
//0 - 'P' or 'R' for press or release
//1 - Octave
//2 - Key
//3 - Volume
//4 - Recording index [Upper] (in recording mode, unused in scan keys)
//5 - Recording index [Lower] (in recording mode, unused in scan keys)
//6 - Playback index (in recording mode, unused in scan keys)


void recordTask(void * pwParameters){
    const TickType_t xFrequency = 10/portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t playbackBuffer[][8];
    uint8_t RX_Prev[8];
    uint8_t RX_Local[8];
    bool recordButtonPrevious = 1; //Button 1, index 25
    bool playbackButtonPrevious = 1; //Button 2, index 20
    bool playback = false;
    bool recording = false;
    bool slaveLocal;
    uint8_t track = 0; //Uses one hot encoding to show active tracks
    uint16_t counter = 0;
    uint8_t counterUpper;
    uint8_t counterLower;
    uint16_t mmaxTime = 3000;//30s max recording time
    while (1){
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
        counterUpper = (uint8_t) (counter & 0xFF00) >> 8;
        counterLower = (uint8_t) (counter & 0x00FF);

        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        if (sysState.inputs[25] != recordButtonPrevious){//Enable recording
            recordButtonPrevious = sysState.inputs[25];
            if (sysState.inputs[25] == 0) recording = !recording;
        }
        if (sysState.inputs[20] != playbackButtonPrevious){//Enable recording
            playbackButtonPrevious = sysState.inputs[25];
            if (sysState.inputs[20] == 0) playback = !playback;
        }
        slaveLocal = sysState.slave;
        xSemaphoreGive(sysState.mutex);
        //NEED TO ADD TRACK SELECTION

        if (counter > mmaxTime){
            counter = 0;
            // playback = false; //Uncomment  this to prevent playback from looping
            if (recording){
                recording = false;
                // std::sort(playbackBuffer.begin(), playbackBuffer.end(), compareNotesIndex); //Sort notes by counter index
            }
        }
        if (recording){
            xSemaphoreTake(sysState.mutex, portMAX_DELAY);
            for (int i = 0; i < 8; i++) RX_Local[i] = sysState.RX_Message[i];//Saves message for printing
            xSemaphoreGive(sysState.mutex);
            if (RX_Local != RX_Prev){
                RX_Local[4] = counterUpper;    //Upper 8 bits
                RX_Local[5] = counterLower;         //lower 8 bits
                RX_Local[6] = track;
                std::array<uint8_t,8> RX_Local_Array;
                std::copy(std::begin(RX_Local), std::end(RX_Local), RX_Local_Array.begin());
                playbackBuffer[track].push_back(RX_Local);
                for (int i = 0; i < 8; i++) RX_Prev[i] = RX_Local[i];
            }
            counter++;
        }


        //This will be terribly ineffecient, may need to consider a better way to do this. 
        if (playback){
            if (track & 0b0001){
                for (int i = 0; i < playbackBuffer[0].size(); i++){
                    if (counterUpper == playbackBuffer[0][i][4] && counterLower == playbackBuffer[0][i][5]){
                        if (slaveLocal) xQueueSend(msgOutQ,playbackBuffer[0][i],portMAX_DELAY);
                        else xQueueSend(msgInQ,playbackBuffer[0][i],portMAX_DELAY);
                    }
                }
            }
            if (track & 0b0010){
                for (int i = 0; i < playbackBuffer[1].size(); i++){
                    if (counterUpper == playbackBuffer[1][i][4] && counterLower == playbackBuffer[1][i][5]){
                        if (slaveLocal) xQueueSend(msgOutQ,playbackBuffer[1][i],portMAX_DELAY);
                        else xQueueSend(msgInQ,playbackBuffer[1][i],portMAX_DELAY);
                    }
                }
            }
            if (track & 0b0100){
                
                for (int i = 0; i < playbackBuffer[2].size(); i++){
                    if (counterUpper == playbackBuffer[2][i][4] && counterLower == playbackBuffer[2][i][5]){
                        if (slaveLocal) xQueueSend(msgOutQ,playbackBuffer[2][i],portMAX_DELAY);
                        else xQueueSend(msgInQ,playbackBuffer[2][i],portMAX_DELAY);
                    }
                }
            }
            if (track & 0b1000){
                for (int i = 0; i < playbackBuffer[3].size(); i++){
                    if (counterUpper == playbackBuffer[3][i][4] && counterLower == playbackBuffer[3][i][5]){
                        if (sysState.slave) xQueueSend(msgOutQ,playbackBuffer[3][i],portMAX_DELAY);
                        else xQueueSend(msgInQ,playbackBuffer[3][i],portMAX_DELAY);
                    }
                }
            }
            counter++;
        }

        
    }   
}