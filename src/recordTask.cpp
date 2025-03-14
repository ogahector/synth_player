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
    std::vector<uint8_t [8]> playbackBuffer;
    uint8_t RX_Prev[8];
    uint8_t RX_Local[8];
    bool recordButtonPrevious = 1; //Button 1, index 25
    bool playbackButtonPrevious = 1; //Button 2, index 20
    bool playback = false;
    bool recording = false;
    uint8_t track = 0x01; //Gonna use 1 hot encoding for track selection (e.g. 1, 2, 4 and 8)
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
                // playbackBuffer.push_back(RX_Local);
                for (int i = 0; i < 8; i++) RX_Prev[i] = RX_Local[i];
            }
            counter++;
        }


        //This will be terribly ineffecient, may need to consider a better way to do this. 
        if (playback){
            // xSemaphoreTake(notesBuffer.mutex, portMAX_DELAY);
            for (int i = 0; i < playbackBuffer.size(); i++){
                if (playbackBuffer[i][4] == counterUpper && playbackBuffer[i][5] == counterLower){
                    //Can hopefully just add it to the CAN bus (WON'T WORK FOR ITS OWN VALUES YET)
                    xQueueSend( msgOutQ, playbackBuffer[i], portMAX_DELAY);
                }
            }
            counter++;
        }

        
    }   
}

bool compareNotesIndex(uint8_t note1[8], uint8_t note2[8]){
    uint16_t index1 = ((uint16_t) note1[4] << 8) | note1[5];
    uint16_t index2 = ((uint16_t) note2[4] << 8) | note2[5];
    return index1 < index2;
}