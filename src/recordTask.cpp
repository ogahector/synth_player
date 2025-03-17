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
    std::vector<std::vector<std::array<uint8_t,8> > > playbackBuffer(4);
    uint32_t playbackPointers[4] = {0,0,0,0};
    for (int i = 0; i < playbackBuffer.size() ; i++){
        playbackBuffer[i].reserve(100); //Reserves 100 spcaes in memory for each track (0.4kB overall)
    }
    uint8_t RX_Prev[8];
    uint8_t RX_Local[8];
    bool recordButtonPrevious = 1; //Button 1, index 25
    bool playbackButtonPrevious = 1; //Button 2, index 20
    bool playback = false;
    bool recording = false;
    bool slaveLocal;
    uint8_t track = 0b0001; //Uses one hot encoding to show active tracks
    uint16_t counter = 0;
    uint8_t counterUpper;
    uint8_t counterLower;
    uint16_t mmaxTime = 1000;//10s max recording time
    while (1){
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
        counterUpper = (uint8_t) (counter >> 8);
        counterLower = (uint8_t) (counter & 0xFF);

        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        if (sysState.inputs[25] != recordButtonPrevious){//Enable recording (Knob 1)
            recordButtonPrevious = sysState.inputs[25];
            if (sysState.inputs[25] == 0) {
                if (recording) {
                    Serial.println("Recording ended, printing queue...");
                    Serial.println(playbackBuffer[0].size());
                    counter = 0;
                    for (int i = 0; i < playbackBuffer[0].size(); i++){
                        Serial.print("Index : ");
                        Serial.print(i);
                        Serial.print(" - Value : ");
                        Serial.print((char)playbackBuffer[0][i][0]);
                        Serial.print(".");
                        Serial.print(playbackBuffer[0][i][1]);
                        Serial.print(".");
                        Serial.print(playbackBuffer[0][i][2]);
                        Serial.print(".");
                        Serial.print(playbackBuffer[0][i][3]);
                        Serial.print(".");
                        Serial.print(playbackBuffer[0][i][4]);
                        Serial.print(".");
                        Serial.print(playbackBuffer[0][i][5]);
                        Serial.print(".");
                        Serial.print(playbackBuffer[0][i][6]);
                        Serial.print(".");
                        Serial.println(playbackBuffer[0][i][7]);
                    }
                }
                else{
                    playbackBuffer[0].clear();
                }
                recording = !recording;
            }
        }
        if (sysState.inputs[20] != playbackButtonPrevious){//Enable playback (knob 2)
            playbackButtonPrevious = sysState.inputs[20];
            if (sysState.inputs[20] == 0) playback = !playback;
        }
        slaveLocal = sysState.slave;
        xSemaphoreGive(sysState.mutex);
        //NEED TO ADD TRACK SELECTION

        if (counter > mmaxTime){
            Serial.println("Max Time reached, stopping recording/playback");
            counter = 0;
            // playback = false; //Uncomment  this to prevent playback from looping
            if (recording){
                recording = false;
            }
        }
        if (recording){
            // Serial.println("Recording");
            xSemaphoreTake(sysState.mutex, portMAX_DELAY);
            for (int i = 0; i < 8; i++) RX_Local[i] = sysState.RX_Message[i];//Saves message for printing
            xSemaphoreGive(sysState.mutex);
            if (counter == 0) {
                counter++;
                for (int i = 0; i < 8; i++) RX_Prev[i] = RX_Local[i];
                continue;
            }
            if (RX_Local[0] != RX_Prev[0] || RX_Local[1] != RX_Prev[1] || RX_Local[2] != RX_Prev[2]){//Since prev will have different 4,5 and 6 values
                Serial.print("New : ");
                Serial.print((char)RX_Local[0]);
                Serial.print(".");
                Serial.print((int)RX_Local[1]);
                Serial.print(".");
                Serial.print((int)RX_Local[2]);
                Serial.print(" - Counter : ");
                Serial.print(counter);
                Serial.print(" - Upper : ");
                Serial.print(counterUpper);
                Serial.print(" - Lower : ");
                Serial.println(counterLower);
                RX_Local[4] = counterUpper;    //Upper 8 bits
                RX_Local[5] = counterLower;         //lower 8 bits
                RX_Local[6] = track;
                std::array<uint8_t,8> RX_Local_Array;
                std::copy(std::begin(RX_Local), std::end(RX_Local), RX_Local_Array.begin());
                //Add logic here to choose index based off track
                playbackBuffer[0].push_back(RX_Local_Array);
                for (int i = 0; i < 8; i++) RX_Prev[i] = RX_Local[i];
                Serial.println("Added key to queue");
            }
            counter++;
        }


        //This will be terribly ineffecient, may need to consider a better way to do this. 
        if (playback){
            if (track & 0b0001){
                uint16_t index = playbackBuffer[0][playbackPointers[0]][4] << 8 | (playbackBuffer[0][playbackPointers[0]][5]);
                if (counter == index){
                    Serial.println("Sending playback");
                    for (int i = 0; i < 8; i++) RX_Local[i] = playbackBuffer[0][playbackPointers[0]][i];
                    if (slaveLocal) xQueueSend(msgOutQ,RX_Local,portMAX_DELAY);
                    else xQueueSend(msgInQ,RX_Local,portMAX_DELAY);
                    playbackPointers[0] = (playbackPointers[0] + 1) % playbackBuffer[0].size();
                }
            }
            // if (track & 0b0010){

            // }
            // if (track & 0b0100){
                
            // }
            // if (track & 0b1000){

            // }
            counter++;
        }

        
    }   
}