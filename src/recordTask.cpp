#include <recordTask.h>

//RX_Message values:
//0 - 'P' or 'R' for press or release
//1 - Octave
//2 - Key
//3 - Volume
//4 - Recording index [Upper] (in recording mode, unused in scan keys)
//5 - Recording index [Lower] (in recording mode, unused in scan keys)

void addNote(uint8_t track, uint8_t octave, uint8_t key, std::vector<std::vector<std::pair<uint8_t,uint8_t>>> &activeNotes) {
    // Create the note as a pair
    auto note = std::make_pair(octave, key);
    
    // Check if the note is already present
    if (std::find(activeNotes[track].begin(), activeNotes[track].end(), note) == activeNotes[track].end()) {
        activeNotes[track].push_back(note); // Add only if not found
    }
}

void removeNote(uint8_t track, uint8_t octave, uint8_t key, std::vector<std::vector<std::pair<uint8_t,uint8_t>>> &activeNotes) {
    auto note = std::make_pair(octave, key);
    auto it = std::find(activeNotes[track].begin(), activeNotes[track].end(), note);
    if (it != activeNotes[track].end()) {
        activeNotes[track].erase(it); // Remove the note
    }
}

void releaseAllNotes(uint8_t track, QueueHandle_t queue, std::vector<std::vector<std::pair<uint8_t,uint8_t>>> &activeNotes) {
    for (const auto& note : activeNotes[track]) {
        uint8_t releaseEvent[8] = {'R', note.first, note.second, 0, 0, 0, 0, 0};
        xQueueSend(queue, releaseEvent, portMAX_DELAY); // Send to queue (e.g., FreeRTOS)
    }
    activeNotes[track].clear(); // Reset the track
}

void recordTask(void * pwParameters){
    const TickType_t xFrequency = 3/portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    std::vector<std::vector<std::pair<uint8_t, uint8_t>>> activeNotes(4);
    std::vector<std::vector<std::array<uint8_t,8> > > playbackBuffer(4);
    uint32_t playbackPointers[4] = {0,0,0,0};
    for (int i = 0; i < 4 ; i++){
        playbackBuffer[i].reserve(100); //Reserves 100 spcaes in memory for each track (0.4kB overall)
        activeNotes.reserve(20); //Realistically not going to exceed 20 keys 
    }
    uint8_t RX_Prev[8];
    uint8_t RX_Local[8];
    uint8_t track_to_stop;
    bool playback = false;
    bool recording = false;
    bool slaveLocal;
    uint8_t active_tracks = 0b0001; //Uses one hot encoding to show active tracks
    uint8_t track = 0;
    uint16_t counter = 0;
    uint8_t counterUpper;
    uint8_t counterLower;
    uint16_t mmaxTime = 1667;//5s max recording time
    while (1){
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
        counterUpper = (uint8_t) (counter >> 8);
        counterLower = (uint8_t) (counter & 0xFF);

        xSemaphoreTake(sysState.mutex,portMAX_DELAY);
        slaveLocal = sysState.slave;
        xSemaphoreGive(sysState.mutex);

        xSemaphoreTake(record.mutex,portMAX_DELAY);
        
        track = record.current_track;
        active_tracks = record.active_tracks;
        if (recording && !record.recording) {//Stop recording
            recording = false;
            releaseAllNotes(track, slaveLocal ? msgOutQ : msgInQ, activeNotes);
            if (!playbackBuffer[track].empty() && playbackBuffer[track].back()[0] == 'P') {
                playbackBuffer[track].pop_back(); // Remove unpaired press
            }
        }
        else if (!recording && record.recording){//Start recording
            recording = true;
            counter = 0;
            releaseAllNotes(track, slaveLocal ? msgOutQ : msgInQ, activeNotes);
            playbackBuffer[track].clear();
            Serial.println("Cleared");
        }
        if (playback && !record.playback){//Stop playback
            playback = false;
            releaseAllNotes(track, slaveLocal ? msgOutQ : msgInQ, activeNotes);
        }
        else if(!playback && record.playback){//Stop playback
            playback = true;
            counter = 0;
            for (int i = 0; i < 4; i++) {
                playbackPointers[i] = 0;
            }
        }
        
        xSemaphoreGive(record.mutex);

        //NEED TO ADD TRACK SELECTION

        if (counter > mmaxTime){
            Serial.println("Max Time");
            counter = 0;
            // playback = false; //Uncomment  this to prevent playback from looping
            if (recording){
                Serial.println("Recording ended (Max time)");
                recording = false;
                xSemaphoreTake(record.mutex,portMAX_DELAY);
                record.recording = false;
                xSemaphoreGive(record.mutex);
                releaseAllNotes(track, slaveLocal ? msgOutQ : msgInQ, activeNotes);
                if (!playbackBuffer[track].empty() && playbackBuffer[track].back()[0] == 'P') {
                    playbackBuffer[track].pop_back(); // Remove unpaired press
                }
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
            if (RX_Local[4] == 0 && RX_Local[5] == 0){//This will prevent it from recording other tracks when playing multiple while recording
                if ((RX_Local[0] != RX_Prev[0]) || (RX_Local[1] != RX_Prev[1]) || (RX_Local[2] != RX_Prev[2]) ){//Since prev will have different 4,5 and 6 values
                    RX_Local[4] = counterUpper;    //Upper 8 bits
                    RX_Local[5] = counterLower;    //lower 8 bits
                    std::array<uint8_t,8> RX_Local_Array;
                    std::copy(std::begin(RX_Local), std::end(RX_Local), RX_Local_Array.begin());
                    playbackBuffer[track].push_back(RX_Local_Array);

                    //Add to activeNotes
                    if (RX_Local[0] == 'P') {
                        addNote(track, RX_Local[1], RX_Local[2], activeNotes);
                    } else if (RX_Local[0] == 'R') {
                        removeNote(track, RX_Local[1], RX_Local[2], activeNotes);
                    }
                    for (int i = 0; i < 8; i++) RX_Prev[i] = RX_Local[i];
                }
            }
            
        }



        if (playback){
            if (active_tracks & 0b0001){
                uint16_t index = playbackBuffer[0][playbackPointers[0]][4] << 8 | (playbackBuffer[0][playbackPointers[0]][5]);
                if (counter == index){
                    for (int i = 0; i < 8; i++) RX_Local[i] = playbackBuffer[0][playbackPointers[0]][i];
                    if (slaveLocal) xQueueSend(msgOutQ,RX_Local,portMAX_DELAY);
                    else xQueueSend(msgInQ,RX_Local,portMAX_DELAY);
                    if (RX_Local[0] == 'P') {
                        addNote(track, RX_Local[1], RX_Local[2], activeNotes);
                    } else if (RX_Local[0] == 'R') {
                        removeNote(track, RX_Local[1], RX_Local[2], activeNotes);
                    }
                    playbackPointers[0] = (playbackPointers[0] + 1) % playbackBuffer[0].size();
                }
            }
            if (active_tracks & 0b0010){
                uint16_t index = playbackBuffer[1][playbackPointers[1]][4] << 8 | (playbackBuffer[1][playbackPointers[1]][5]);
                if (counter == index){
                    for (int i = 0; i < 8; i++) RX_Local[i] = playbackBuffer[1][playbackPointers[1]][i];
                    if (slaveLocal) xQueueSend(msgOutQ,RX_Local,portMAX_DELAY);
                    else xQueueSend(msgInQ,RX_Local,portMAX_DELAY);
                    if (RX_Local[0] == 'P') {
                        addNote(track, RX_Local[1], RX_Local[2], activeNotes);
                    } else if (RX_Local[0] == 'R') {
                        removeNote(track, RX_Local[1], RX_Local[2], activeNotes);
                    }
                    playbackPointers[1] = (playbackPointers[1] + 1) % playbackBuffer[1].size();
                }
            }
            if (active_tracks & 0b0100){
                uint16_t index = playbackBuffer[2][playbackPointers[2]][4] << 8 | (playbackBuffer[2][playbackPointers[2]][5]);
                if (counter == index){
                    for (int i = 0; i < 8; i++) RX_Local[i] = playbackBuffer[2][playbackPointers[2]][i];
                    if (slaveLocal) xQueueSend(msgOutQ,RX_Local,portMAX_DELAY);
                    else xQueueSend(msgInQ,RX_Local,portMAX_DELAY);
                    if (RX_Local[0] == 'P') {
                        addNote(track, RX_Local[1], RX_Local[2], activeNotes);
                    } else if (RX_Local[0] == 'R') {
                        removeNote(track, RX_Local[1], RX_Local[2], activeNotes);
                    }
                    playbackPointers[2] = (playbackPointers[2] + 1) % playbackBuffer[2].size();
                }
            }
            if (active_tracks & 0b1000){
                uint16_t index = playbackBuffer[3][playbackPointers[3]][4] << 8 | (playbackBuffer[3][playbackPointers[3]][5]);
                if (counter == index){
                    for (int i = 0; i < 8; i++) RX_Local[i] = playbackBuffer[3][playbackPointers[3]][i];
                    if (slaveLocal) xQueueSend(msgOutQ,RX_Local,portMAX_DELAY);
                    else xQueueSend(msgInQ,RX_Local,portMAX_DELAY);
                    if (RX_Local[0] == 'P') {
                        addNote(track, RX_Local[1], RX_Local[2], activeNotes);
                    } else if (RX_Local[0] == 'R') {
                        removeNote(track, RX_Local[1], RX_Local[2], activeNotes);
                    }
                    playbackPointers[3] = (playbackPointers[3] + 1) % playbackBuffer[3].size();
                }
            }
        }

        if (recording || playback) counter++;
    }   
}