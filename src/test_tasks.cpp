#include <bitset>
#include <array>
#include <knob.h>
#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <globals.h>
#include <menu.h>
#include <waves.h>
#include <home.h>
#include <U8g2lib.h>
#include <scanKeysTask.h>
#include <recording.h>
#include <doom_def.h>
#include <ES_CAN.h>
#include <CANTask.h>
#include <sig_gen.h>
#include <test_tasks.h>
// A test version of scanKeysTask that simulates worst-case processing.
// This version does not block and generates a key press for every key.

sysState_t sysState_Test;

#ifdef TEST_SCANKEYS

Knob K3 = Knob(0,8);//Volume Knob
Knob K2 = Knob(-4,4);//Octave Knob

bool muteReleased = true;
bool slaveReleased = true;
bool menuButton = false;
bool joystickButton = false;
std::array<int, 2> joystickValues;
std::bitset<32> previousInputs;
std::bitset<4> cols;
std::bitset<4> row_cols;
static bool toggle = false;
uint8_t TX_Message[8] = {0};//Message sent over CAN

void testScanKeys(int state_choice) {
  // vTaskDelayUntil( &xLastWakeTime, xFrequency );

  // ####### CHECK MENU TOGGLE #######
  switch (state_choice) {
    case 0 :  
      sysState_Test.activityList = HOME;
      break;
    case 1 :  
      sysState_Test.activityList = MENU;
      break;
    case 2 :
      sysState_Test.activityList = DOOM;
      break;
    case 3 : 
      sysState_Test.activityList = WAVE;
      break;
    case 4 :
      sysState_Test.activityList = RECORDING;
      break;
    default :
      sysState_Test.activityList = HOME;
      break;
  }

  setRow(6);
  delayMicroseconds(3);
  row_cols = readCols();

  // xSemaphoreTake(sysState_Test.mutex, portMAX_DELAY);
  if (!row_cols[0] && menuButton){
    menuButton = false;
    if (toggle) {
      sysState_Test.activityList = HOME;
    } else {
      sysState_Test.activityList = MENU;
    }
    toggle = !toggle;  // flip toggle for next press
  }
  else if (row_cols[0]) menuButton = true;

  switch (sysState_Test.activityList)
  {
    case HOME:
      previousInputs = sysState_Test.inputs;
      for (int i = 0; i < 7; i++){//Read rows
        setRow(i);
        delayMicroseconds(3);
        cols = readCols();
        for (int j = 0; j < 4; j++) sysState_Test.inputs[4*i + j] = cols[j];
      }
      for (int i = 0; i < 12; i++){//Checks all keys
        if (sysState_Test.inputs[i] != previousInputs[i]){//Checks if a NEW key has been pressed
          TX_Message[0] = (sysState_Test.inputs[i] & 0b1) ? 'R' : 'P';
          TX_Message[1] = sysState_Test.Octave + 4;
          TX_Message[2] = i;
          TX_Message[3] = sysState_Test.mute ? 255 : sysState_Test.Volume;
          if (sysState_Test.slave) xQueueSend( msgOutQ, TX_Message, portMAX_DELAY);//Sends via CAN
          #if !LOOPBACK
          if (!sysState_Test.slave) xQueueSend(msgInQ,TX_Message,0); //Updates directly for master
          #endif
        }
      }
      if (!sysState_Test.slave) sysState_Test.Volume = K3.update(sysState_Test.inputs[12], sysState_Test.inputs[13]);//Volume adjustment
      sysState_Test.Octave = K2.update(sysState_Test.inputs[14], sysState_Test.inputs[15]);//Octave Adjustment

      if(!sysState_Test.inputs[21] && muteReleased) {
        muteReleased = false;
        sysState_Test.mute = !sysState_Test.mute;
      }
      else if (sysState_Test.inputs[21]) muteReleased = true;

    // Toggles slave (Knob 2 Press)
      if(!sysState_Test.inputs[20] && slaveReleased) {
        slaveReleased = false;
        sysState_Test.slave = !sysState_Test.slave;
      }
      else if (sysState_Test.inputs[20]) slaveReleased = true;
      break;

    case MENU:
      joystickValues = joystickRead();
      sysState_Test.joystickHorizontalDirection = joystickValues[0];
      setRow(5);
      delayMicroseconds(3);
      row_cols = readCols();
      if(!row_cols[2] && joystickButton) {
        joystickButton = false;
        sysState_Test.joystickPress = true;
      }
      else if (row_cols[2]) joystickButton = true;
      break;
    
    case DOOM:
      joystickValues = joystickRead();
      sysState_Test.joystickHorizontalDirection = joystickValues[0];
      sysState_Test.joystickVerticalDirection = joystickValues[1];
      setRow(5);
      delayMicroseconds(3);
      row_cols = readCols();
      if(!row_cols[2]) {
        sysState_Test.joystickPress = true;
      }
      else if (row_cols[2]) sysState_Test.joystickPress = false;
      break;
    case WAVE:
      joystickValues = joystickRead();
      sysState_Test.joystickHorizontalDirection = joystickValues[0];
      sysState_Test.joystickVerticalDirection = joystickValues[1];
      setRow(5);
      delayMicroseconds(3);
      row_cols = readCols();
      // Treat the joystick press as home button press
      if(!row_cols[2] && joystickButton) {
        joystickButton = false;
        menuButton = false;
        toggle=false;
        sysState_Test.activityList = HOME;
      }
      else if (row_cols[2]) {
        joystickButton = true;
        menuButton = true;
      }
      break;
    case RECORDING:
      previousInputs = sysState_Test.inputs;
      for (int i = 0; i < 7; i++){//Read rows
        setRow(i);
        delayMicroseconds(3);
        cols = readCols();
        for (int j = 0; j < 4; j++) sysState_Test.inputs[4*i + j] = cols[j];
      } 
      for (int i = 0; i < 12; i++){//Checks all keys
        if (sysState_Test.inputs[i] != previousInputs[i]){//Checks if a NEW key has been pressed
          TX_Message[0] = (sysState_Test.inputs[i] & 0b1) ? 'R' : 'P';
          TX_Message[1] = sysState_Test.Octave + 4;
          TX_Message[2] = i;
          TX_Message[3] = sysState_Test.mute ? 255 : sysState_Test.Volume;
          if (sysState_Test.slave) xQueueSend( msgOutQ, TX_Message, portMAX_DELAY);//Sends via CAN
          #if !LOOPBACK
          if (!sysState_Test.slave) xQueueSend(msgInQ,TX_Message,0); //Updates directly for master
          #endif
        }
      }
      if (!sysState_Test.slave) sysState_Test.Volume = K3.update(sysState_Test.inputs[12], sysState_Test.inputs[13]);//Volume adjustment
      sysState_Test.Octave = K2.update(sysState_Test.inputs[14], sysState_Test.inputs[15]);//Octave Adjustment
      joystickValues = joystickRead();
      sysState_Test.joystickHorizontalDirection = joystickValues[0];
      sysState_Test.joystickVerticalDirection = joystickValues[1];
      setRow(5);
      delayMicroseconds(3);
      row_cols = readCols();
      if(!row_cols[2] && joystickButton) {
        joystickButton = false;
        sysState_Test.joystickPress = true;
      }
      else if (row_cols[2]) joystickButton = true;
    default:
      break;
  }
}
#endif

#ifdef TEST_DISPLAYUPDATE
bool doomLoadingShown1=false;
bool alreadyShown1=false;
int localActivity = -1;
int localJoystickDir = 0;
static int previousActivity = -1;


  
  void testDisplayUpdate(int state_choice)
  {
    switch (state_choice) {
      case 0 :  
        sysState_Test.activityList = HOME;
        break;
      case 1 :  
        sysState_Test.activityList = MENU;
        break;
      case 2 :
        sysState_Test.activityList = DOOM;
        break;
      case 3 : 
        sysState_Test.activityList = WAVE;
        break;
      case 4 :
        sysState_Test.activityList = RECORDING;
        break;
      default :
        sysState_Test.activityList = HOME;
        break;
    }
    switch (sysState_Test.activityList)
    {
      case MENU:
        localActivity = 1;
        break;
      case DOOM:
        localActivity = 2;
        break;
      case WAVE:
        localActivity = 3;
        break;
      case RECORDING:
        localActivity = 4;
        break;
      case HOME:
        localActivity = 0;
        break;
      default:
        localActivity = -1;
        break;
    }
    localJoystickDir = sysState_Test.joystickHorizontalDirection;
    // xSemaphoreGive(sysState_Test.mutex);


    if (localActivity == 2)
    {
      // Transitioning into doom state: if we weren't in doom previously, trigger loading screen.
      if (previousActivity != 2) {
         doomLoadingShown1 = false; // Show loading screen
      }
      else {
         doomLoadingShown1 = true;  // Already in doom; no need to show the loading screen again.
      }
    }
    else {
      // For non-doom activities, reset the flag so that if we return to doom, the loading screen appears.
      doomLoadingShown1 = true;
    }
    if (localActivity == 4)
    {
      // Transitioning into doom state: if we weren't in doom previously, trigger loading screen.
      if (previousActivity != 4) {
        alreadyShown1 = false; // Show loading screen
      }
      else {
        alreadyShown1 = true;  // Already in doom; no need to show the loading screen again.
      }
    }
    else {
      // For non-doom activities, reset the flag so that if we return to doom, the loading screen appears.
      alreadyShown1 = true;
    }
    // Now, outside the critical section, do the rendering.
    switch (localActivity)
    {
      case 1:
        renderMenu();
        break;
      case 2:
        renderDoomScene(doomLoadingShown1);
        break;
      case 3: {
        int selection = renderWaves();
        // xSemaphoreTake(sysState_Test.mutex, portMAX_DELAY);
        sysState_Test.currentWaveform = static_cast<waveform_t>(selection);
        // xSemaphoreGive(sysState_Test.mutex);
        break;
      }
      case 4:
        renderRecording(alreadyShown1);
        break;
      case 0:
        renderHome();
        break;
      default:
        u8g2.clearBuffer();
        u8g2.setCursor(0, 10);
        u8g2.print("There was an error");
        u8g2.sendBuffer();
        break;
    }
    previousActivity = localActivity;
  }
#endif

#ifdef TEST_DECODE

static std::pair<uint8_t,uint8_t> incoming;

void testDecode(int state){
  uint8_t RX_Message[8] = {0,0,0,0,0,0,0,0};
  switch (state){
    case 0 :
      RX_Message[0] = 'P';
      break;
    case 1 :
      RX_Message[0] = 'R';
      break;
    default :
      RX_Message[0] = 'R';
      break;
  }
    // xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);//Gets current message from queue
    //Also now realise this is basc just the notesPlayed lol
    //May also want to add logic to prevent repeat keys (if this becomes an issue)
    incoming = std::make_pair(RX_Message[1],RX_Message[2]);
    // if (uxSemaphoreGetCount(voices.mutex) == 0){
    //     Serial.println("Voices locked (Decode)");
    // }
    // xSemaphoreTake(voices.mutex,portMAX_DELAY);
    if (RX_Message[0] == 'P'){
        voices.voices_array[incoming.first * 12 + incoming.second].phaseAcc = 0;
        voices.notes.push_back(incoming);
    }
    else if (RX_Message[0] == 'R'){
        for (int i = 0; i < voices.notes.size(); i++){
            if (voices.notes[i] == incoming) {
                voices.notes[i] = voices.notes.back();
                voices.notes.pop_back();
                break;
            }
        }
    }
    // xSemaphoreGive(voices.mutex);

    // xSemaphoreTake(sysState_Test.mutex, portMAX_DELAY);
    if (sysState_Test.slave) {//Handles slave muting
        if (RX_Message[3] == 0xFF) sysState_Test.mute = true;
        else {
            sysState_Test.mute = false;
            sysState_Test.Volume = RX_Message[3];
        }
    }
    for (int i = 0; i < 8; i++) sysState_Test.RX_Message[i] = RX_Message[i];//Saves message for printing
    // xSemaphoreGive(sysState_Test.mutex);
}
#endif

#ifdef TEST_TRANSMIT
//Useless, it all blocks I believe
uint8_t msgOut[8];
void testTransmit(){
    // xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
    // xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);
    CAN_TX(0x123, msgOut);
}
#endif

#ifdef TEST_SIGGEN
//Stand in buffer 
uint8_t test_buffer[DAC_BUFFER_SIZE];

static waveform_t currentWaveformLocal;
static bool writeBuffer1Local = false;
uint8_t Vout;
int volumeLocal;
bool muteLocal;

const uint8_t shiftBits = 32 - LUT_BITS;

uint8_t sineWave1[LUT_SIZE];

uint8_t squareWave1[LUT_SIZE];

uint8_t triangleWave1[LUT_SIZE];

uint8_t sawtoothWave1[LUT_SIZE];

void computeValues(){
  //Precompute values
  //Sine
  for (size_t i = 0; i < LUT_SIZE; i++) {
    // Convert the index to an angle (radians)
    double angle = (2.0 * M_PI * i) / LUT_SIZE;
    // Compute sine value, then map from [-1,1] to [0,255]
    sineWave1[i] = (uint8_t)((sin(angle) + 1.0) * ((double) 256 / 2.0));
  }

  //Square
  for (size_t i = 0; i < LUT_SIZE; i++) {
    // For the first half, output 0; for the second half, output 255.
    squareWave1[i] = (i < LUT_SIZE / 2) ? 0 : 255;
  }

  //Triangle
  for (size_t i = 0; i < LUT_SIZE; i++) {
    if (i < LUT_SIZE/2) {
        // Linear ramp up: i from 0 to half-1 maps to 0 to 255.
        double value = (double)(i) / (LUT_SIZE - 1) * 255.0;
        triangleWave1[i] = (uint8_t)(value);
    } else {
        // Linear ramp down: i from half to table.size()-1 maps to 255 down to 0.
        double value = (double)(LUT_SIZE- 1 - i) / (LUT_SIZE - 1) * 255.0;
        triangleWave1[i] = (uint8_t)(value);
    }
  }

  //Sawtooth
  for (size_t i = 0; i < LUT_SIZE; i++) {
    double value = (double)(i) / (LUT_SIZE - 1) * 255.0;
    sawtoothWave1[i] = (uint8_t)(value);
  }
}




void testSigGen(int wave){
    // xSemaphoreTake(sysState_Test.mutex, portMAX_DELAY);
    // __atomic_load(&sysState_Test.currentWaveform, &currentWaveformLocal, __ATOMIC_RELAXED);
    // __atomic_load(&sysState_Test.Volume, &volumeLocal, __ATOMIC_RELAXED);
    // __atomic_load(&sysState_Test.mute, &muteLocal, __ATOMIC_RELAXED);
    // xSemaphoreGive(sysState_Test.mutex);
    // if (uxSemaphoreGetCount(signalBufferSemaphore) == 0){
    //     Serial.println("Mutex Locked (Written and waiting)");
    // }
    // else{
    //     Serial.println("Mutex Available (Waitng for write)");
    // }


    // xSemaphoreTake(signalBufferSemaphore, portMAX_DELAY);

    // __atomic_load(&writeBuffer1, &writeBuffer1Local, __ATOMIC_RELAXED);

    volumeLocal = 8;
    muteLocal = false;
    switch (wave)
    {
    case 0:
      currentWaveformLocal = SINE;
      break;
    case 1:
      currentWaveformLocal = SQUARE;
      break;
    case 2:
      currentWaveformLocal = SAWTOOTH;
      break;
    case 3:
      currentWaveformLocal = TRIANGLE;
      break;
    default:
      currentWaveformLocal = SINE;
      break;
    }

    // dac_write_HEAD = writeBuffer1 ? dac_buffer : &dac_buffer[HALF_DAC_BUFFER_SIZE];


    // memset((uint8_t*) dac_write_HEAD, (uint8_t) 0, HALF_DAC_BUFFER_SIZE); // clear buffer much faster
    // for(size_t i = 0; i < HALF_DAC_BUFFER_SIZE; i++) dac_write_HEAD[i] = 0;

    if(muteLocal) return;

    fillBufferTest(currentWaveformLocal, test_buffer, HALF_DAC_BUFFER_SIZE,volumeLocal);

}

inline void fillBufferTest(waveform_t wave, volatile uint8_t buffer[], uint32_t size, int volume) {
  static uint8_t voiceIndex;
  static uint32_t waveIndex;
  // if (uxSemaphoreGetCount(voices.mutex) == 0){
  //     Serial.println("Voices locked (fillBuffer)");
  // }
  // xSemaphoreTake(voices.mutex, portMAX_DELAY);

  // Select the lookup table based on the waveform type.
  uint8_t *waveformLUT;
  switch (wave) {
      case SAWTOOTH:
          waveformLUT = sawtoothWave1;
          break;
      case SINE:
          waveformLUT = sineWave1;
          break;
      case SQUARE:
          waveformLUT = squareWave1;
          break;
      case TRIANGLE:
          waveformLUT = triangleWave1;
          break;
      default:
          waveformLUT = sineWave1;
          break;
  }

  // For each sample in the buffer...
  for (uint32_t i = 0; i < size; i++) {
      uint32_t sampleSum = 0;
      for (int voiceIndex = 0; voiceIndex < 20; voiceIndex++){//For each note currently played
          voices.voices_array[voiceIndex].phaseAcc += voices.voices_array[voiceIndex].phaseInc;//Increment phase accum
          waveIndex = voices.voices_array[voiceIndex].phaseAcc >> shiftBits;//Get wave index
          uint8_t sample = waveformLUT[waveIndex];
          sampleSum += sample >> (8 - volume);//Add to sample
      }
      // Write the summed sample to the output buffer.
      // saturate2uint8_t(sampleSum,0,UINT8_MAX);
      buffer[i] = static_cast<uint8_t>(sampleSum);
  }
  // xSemaphoreGive(voices.mutex);
}
#endif

#ifdef TEST_RECORD



std::vector<std::vector<std::array<uint8_t,8> > > playbackBuffer(4);
std::vector<std::vector<std::pair<uint8_t, uint8_t>>> activeNotes(4);
uint32_t playbackPointers[4] = {0,0,0,0};
uint8_t RX_Prev[8];
uint8_t RX_Local[8];
bool recordButtonPrevious = 1; //Button 1, index 25
bool playbackButtonPrevious = 1; //Button 2, index 20
bool playback = false;
bool recording = false;
bool slaveLocal;
uint8_t active_tracks = 0b0001; //Uses one hot encoding to show active tracks
uint8_t track = 0;
uint16_t counter = 0;
uint8_t counterUpper;
uint8_t counterLower;
uint16_t mmaxTime = 1000;//5s max recording time

void fillPLayback(){
  for (int i = 0; i < playbackBuffer.size() ; i++){
    playbackBuffer[i].reserve(100); //Reserves 100 spcaes in memory for each track (0.4kB overall)
  }
  for (int i = 0; i < 4; i++){
    for (int j = 0; j < 101; j++){
      std::array<uint8_t,8> in = {1,2,3,4,5,6,7};
      playbackBuffer[i].push_back(in);
    }
  }
}

void addNoteTest(uint8_t track, uint8_t octave, uint8_t key, std::vector<std::vector<std::pair<uint8_t,uint8_t>>> &activeNotes) {
  // Create the note as a pair
  auto note = std::make_pair(octave, key);
  
  // Check if the note is already present
  if (std::find(activeNotes[track].begin(), activeNotes[track].end(), note) == activeNotes[track].end()) {
      activeNotes[track].push_back(note); // Add only if not found
  }
}

void removeNoteTest(uint8_t track, uint8_t octave, uint8_t key, std::vector<std::vector<std::pair<uint8_t,uint8_t>>> &activeNotes) {
  auto note = std::make_pair(octave, key);
  auto it = std::find(activeNotes[track].begin(), activeNotes[track].end(), note);
  if (it != activeNotes[track].end()) {
      activeNotes[track].erase(it); // Remove the note
  }
}

void releaseAllNotesTest(uint8_t track, QueueHandle_t queue, std::vector<std::vector<std::pair<uint8_t,uint8_t>>> &activeNotes) {
  for (const auto& note : activeNotes[track]) {
      uint8_t releaseEvent[8] = {'R', note.first, note.second, 0, 0, 0, 0, 0};
      xQueueSend(queue, releaseEvent, portMAX_DELAY); // Send to queue (e.g., FreeRTOS)
  }
  activeNotes[track].clear(); // Reset the track
}

void testRecord(int state){
  counterUpper = (uint8_t) (counter >> 8);
  counterLower = (uint8_t) (counter & 0xFF);
  
  slaveLocal = false;
  active_tracks = 0b1111;
  track = 0;
  switch (state){
    case 0 :
      recording = true;
      playback = false;
      playbackBuffer[0].clear();
      break;
    case 1 : 
      recording = false;
      playback = true;

      break;
    case 2 :
      recording = true;
      playback = true;
      playbackBuffer[0].clear();
      break;
    default :
      recording = false;
      playback = false;
      break;
  }

  if (counter > mmaxTime){            
      // playback = false; //Uncomment  this to prevent playback from looping
      if (recording){
          recording = false;
          record.recording = false;
          if (!activeNotes[track].empty()){
              for (int i = 0; i < activeNotes[track].size(); i++){
                  uint16_t count_temp = mmaxTime - (activeNotes[track].size() + 5) + i;
                  std::array<uint8_t,8> RX_temp;
                  RX_temp[0] = 'R';
                  RX_temp[1] = activeNotes[track][i].first;
                  RX_temp[2] = activeNotes[track][i].second;
                  RX_temp[4] = (uint8_t) (count_temp >> 8);
                  RX_temp[5] = (uint8_t) (count_temp & 0xFF);
                  playbackBuffer[track].push_back(RX_temp);
              }
          }
          
      }
      releaseAllNotesTest(track, slaveLocal ? msgOutQ : msgInQ, activeNotes);
      counter = 0;
      for (int i = 0; i < 4; i++) {
          playbackPointers[i] = 0;
      }
  }
  if (recording){
      RX_Local[0] = 'P';
      RX_Local[1] = counterUpper;
      if (RX_Local[4] == 0 && RX_Local[5] == 0){//This will prevent it from recording other tracks when playing multiple while recording
          if ((RX_Local[0] != RX_Prev[0]) || (RX_Local[1] != RX_Prev[1]) || (RX_Local[2] != RX_Prev[2]) ){//Since prev will have different 4,5 and 6 values
              RX_Local[4] = counterUpper;    //Upper 8 bits
              RX_Local[5] = counterLower;    //lower 8 bits
              std::array<uint8_t,8> RX_Local_Array;
              std::copy(std::begin(RX_Local), std::end(RX_Local), RX_Local_Array.begin());
              playbackBuffer[track].push_back(RX_Local_Array);

              //Add to activeNotes
              if (RX_Local[0] == 'P') {
                  addNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
              } else if (RX_Local[0] == 'R') {
                  removeNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
              }
              for (int i = 0; i < 8; i++) RX_Prev[i] = RX_Local[i];
          }
      }
      
  }



  if (playback){
    // if (active_tracks & 0b0001){
    //   uint16_t index = playbackBuffer[0][playbackPointers[0]][4] << 8 | (playbackBuffer[0][playbackPointers[0]][5]);
    //   if (counter == index){
    //     for (int i = 0; i < 8; i++) RX_Local[i] = playbackBuffer[0][playbackPointers[0]][i];
    //     if (slaveLocal) xQueueSend(msgOutQ,RX_Local,portMAX_DELAY);
    //     else xQueueSend(msgInQ,RX_Local,portMAX_DELAY);
    //     if (RX_Local[0] == 'P') {
    //       addNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
    //     } else if (RX_Local[0] == 'R') {
    //       removeNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
    //     }
    //     playbackPointers[0] = (playbackPointers[0] + 1) % playbackBuffer[0].size();
    //   }
    // }
    // if (active_tracks & 0b0010){
    //   uint16_t index = playbackBuffer[1][playbackPointers[1]][4] << 8 | (playbackBuffer[1][playbackPointers[1]][5]);
    //   if (counter == index){
    //     for (int i = 0; i < 8; i++) RX_Local[i] = playbackBuffer[1][playbackPointers[1]][i];
    //     if (slaveLocal) xQueueSend(msgOutQ,RX_Local,portMAX_DELAY);
    //     else xQueueSend(msgInQ,RX_Local,portMAX_DELAY);
    //     if (RX_Local[0] == 'P') {
    //       addNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
    //     } else if (RX_Local[0] == 'R') {
    //       removeNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
    //     }
    //     playbackPointers[1] = (playbackPointers[1] + 1) % playbackBuffer[1].size();
    //   }
    // }
    // if (active_tracks & 0b0100){
    //   uint16_t index = playbackBuffer[2][playbackPointers[2]][4] << 8 | (playbackBuffer[2][playbackPointers[2]][5]);
    //   if (counter == index){
    //     for (int i = 0; i < 8; i++) RX_Local[i] = playbackBuffer[2][playbackPointers[2]][i];
    //     if (slaveLocal) xQueueSend(msgOutQ,RX_Local,portMAX_DELAY);
    //     else xQueueSend(msgInQ,RX_Local,portMAX_DELAY);
    //     if (RX_Local[0] == 'P') {
    //       addNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
    //     } else if (RX_Local[0] == 'R') {
    //       removeNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
    //     }
    //     playbackPointers[2] = (playbackPointers[2] + 1) % playbackBuffer[2].size();
    //   }
    // }
    // if (active_tracks & 0b1000){
    //   uint16_t index = playbackBuffer[3][playbackPointers[3]][4] << 8 | (playbackBuffer[3][playbackPointers[3]][5]);
    //   if (counter == index){
    //     for (int i = 0; i < 8; i++) RX_Local[i] = playbackBuffer[3][playbackPointers[3]][i];
    //     if (slaveLocal) xQueueSend(msgOutQ,RX_Local,portMAX_DELAY);
    //     else xQueueSend(msgInQ,RX_Local,portMAX_DELAY);
    //     if (RX_Local[0] == 'P') {
    //       addNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
    //     } else if (RX_Local[0] == 'R') {
    //       removeNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
    //     }
    //     playbackPointers[3] = (playbackPointers[3] + 1) % playbackBuffer[3].size();
    //   }
    // }

    for (uint8_t t = 0; t < 4; t++){
      if (active_tracks & (1 << t)){
        uint16_t index = playbackBuffer[t][playbackPointers[t]][4] << 8 | (playbackBuffer[t][playbackPointers[t]][5]);
        if (counter == index){
          for (int i = 0; i < 8; i++) RX_Local[i] = playbackBuffer[t][playbackPointers[t]][i];
          if (slaveLocal) xQueueSend(msgOutQ,RX_Local,portMAX_DELAY);
          else xQueueSend(msgInQ,RX_Local,portMAX_DELAY);
          if (RX_Local[0] == 'P') {
            addNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
          } else if (RX_Local[0] == 'R') {
            removeNoteTest(track, RX_Local[1], RX_Local[2], activeNotes);
          }
          playbackPointers[t] = (playbackPointers[t] + 1) % playbackBuffer[t].size();
        }
      }
    }

  }

  if (recording || playback) counter++;
}
#endif