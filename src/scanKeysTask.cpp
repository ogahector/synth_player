// #include <Arduino.h>
#include <bitset>
#include <STM32FreeRTOS.h>
#include <knob.h>
#include <globals.h>
#include <scanKeysTask.h>
#include <array>

inline void updateNotesPlayedFromCANTX(uint8_t RX_Message[8]);


std::bitset<4> readCols(){
    std::bitset<4> result;
    result[0] = digitalRead(C0_PIN);
    result[1] = digitalRead(C1_PIN); 
    result[2] = digitalRead(C2_PIN);
    result[3] = digitalRead(C3_PIN);
    return result;
}

std::array<int, 2> joystickRead(){
  std::array<int, 2> result;
  result[0] = analogRead(JOYX_PIN);
  result[1] = analogRead(JOYY_PIN);
  return result;
}
  
void setRow(uint8_t rowIdx){
    digitalWrite(REN_PIN,LOW);
    digitalWrite(RA0_PIN, (rowIdx & 0x01) ? HIGH : LOW);
    digitalWrite(RA1_PIN, (rowIdx & 0x02) ? HIGH : LOW);
    digitalWrite(RA2_PIN, (rowIdx & 0x04) ? HIGH : LOW);
    digitalWrite(REN_PIN,HIGH);
}

void scanKeysTask(void * pvParameters) {
  const TickType_t xFrequency = 3/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();

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

  while(1){
    vTaskDelayUntil( &xLastWakeTime, xFrequency );

    // ####### CHECK MENU TOGGLE #######
    setRow(6);
    delayMicroseconds(3);
    row_cols = readCols();

    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    if (!row_cols[0] && menuButton){
      menuButton = false;
      if (toggle) {
        sysState.activityList = HOME;
      } else {
        sysState.activityList = MENU;
      }
      toggle = !toggle;  // flip toggle for next press
    }
    else if (row_cols[0]) menuButton = true;

    switch (sysState.activityList)
    {
      case HOME:
        previousInputs = sysState.inputs;
        for (int i = 0; i < 7; i++){//Read rows
          setRow(i);
          delayMicroseconds(3);
          cols = readCols();
          for (int j = 0; j < 4; j++) sysState.inputs[4*i + j] = cols[j];
        }
        for (int i = 0; i < 12; i++){//Checks all keys
          if (sysState.inputs[i] != previousInputs[i]){//Checks if a NEW key has been pressed
            TX_Message[0] = (sysState.inputs[i] & 0b1) ? 'R' : 'P';
            TX_Message[1] = sysState.Octave + 4;
            TX_Message[2] = i;
            TX_Message[3] = sysState.mute ? 255 : sysState.Volume;
            if (sysState.slave) xQueueSend( msgOutQ, TX_Message, portMAX_DELAY);//Sends via CAN
            #if !LOOPBACK
            if (!sysState.slave) xQueueSend(msgInQ,TX_Message,0); //Updates directly for master
            #endif
          }
        }
        if (!sysState.slave) sysState.Volume = K3.update(sysState.inputs[12], sysState.inputs[13]);//Volume adjustment
        sysState.Octave = K2.update(sysState.inputs[14], sysState.inputs[15]);//Octave Adjustment

        if(!sysState.inputs[21] && muteReleased) {
          muteReleased = false;
          sysState.mute = !sysState.mute;
        }
        else if (sysState.inputs[21]) muteReleased = true;
  
      //Toggles slave (Knob 2 Press)
        // if(!sysState.inputs[20] && slaveReleased) {
        //   slaveReleased = false;
        //   sysState.slave = !sysState.slave;
        // }
        // else if (sysState.inputs[20]) slaveReleased = true;
        break;

      case MENU:
        joystickValues = joystickRead();
        sysState.joystickHorizontalDirection = joystickValues[0];
        setRow(5);
        delayMicroseconds(3);
        row_cols = readCols();
        if(!row_cols[2] && joystickButton) {
          joystickButton = false;
          sysState.joystickPress = true;
        }
        else if (row_cols[2]) joystickButton = true;
        break;
      
      case DOOM:
        joystickValues = joystickRead();
        sysState.joystickHorizontalDirection = joystickValues[0];
        sysState.joystickVerticalDirection = joystickValues[1];
        setRow(5);
        delayMicroseconds(3);
        row_cols = readCols();
        if(!row_cols[2]) {
          sysState.joystickPress = true;
        }
        else if (row_cols[2]) sysState.joystickPress = false;
        break;
      case WAVE:
        joystickValues = joystickRead();
        sysState.joystickHorizontalDirection = joystickValues[0];
        sysState.joystickVerticalDirection = joystickValues[1];
        setRow(5);
        delayMicroseconds(3);
        row_cols = readCols();
        // Treat the joystick press as home button press
        if(!row_cols[2] && joystickButton) {
          joystickButton = false;
          menuButton = false;
          toggle=false;
          sysState.activityList = HOME;
        }
        else if (row_cols[2]) {
          joystickButton = true;
          menuButton = true;
        }
        break;
      case RECORDING:
        previousInputs = sysState.inputs;
        for (int i = 0; i < 7; i++){//Read rows
          setRow(i);
          delayMicroseconds(3);
          cols = readCols();
          for (int j = 0; j < 4; j++) sysState.inputs[4*i + j] = cols[j];
        } 
        for (int i = 0; i < 12; i++){//Checks all keys
          if (sysState.inputs[i] != previousInputs[i]){//Checks if a NEW key has been pressed
            TX_Message[0] = (sysState.inputs[i] & 0b1) ? 'R' : 'P';
            TX_Message[1] = sysState.Octave + 4;
            TX_Message[2] = i;
            TX_Message[3] = sysState.mute ? 255 : sysState.Volume;
            if (sysState.slave) xQueueSend( msgOutQ, TX_Message, portMAX_DELAY);//Sends via CAN
            #if !LOOPBACK
            if (!sysState.slave) xQueueSend(msgInQ,TX_Message,0); //Updates directly for master
            #endif
          }
        }
        if (!sysState.slave) sysState.Volume = K3.update(sysState.inputs[12], sysState.inputs[13]);//Volume adjustment
        sysState.Octave = K2.update(sysState.inputs[14], sysState.inputs[15]);//Octave Adjustment
        joystickValues = joystickRead();
        sysState.joystickHorizontalDirection = joystickValues[0];
        sysState.joystickVerticalDirection = joystickValues[1];
        setRow(5);
        delayMicroseconds(3);
        row_cols = readCols();
        if(!row_cols[2] && joystickButton) {
          joystickButton = false;
          sysState.joystickPress = true;
        }
        else if (row_cols[2]) joystickButton = true;
      default:
        break;
    }
  
    xSemaphoreGive(sysState.mutex);
  }
}

inline void updateNotesMaster(uint8_t RX_Message[8])
{
  static std::vector<uint8_t[8]> buffer;
  static std::pair<uint8_t,uint8_t> incoming;
  incoming = std::make_pair(RX_Message[1],RX_Message[2]);
  if (uxSemaphoreGetCount(voices.mutex) == 0){
    Serial.println("Voices locked (updateNotesMaster)");
  }
  xSemaphoreTake(voices.mutex,portMAX_DELAY);
  if (RX_Message[0] == 'P'){
    voices.notes.push_back(incoming);
  }
  else if (RX_Message[0] == 'R'){
    for (int i = 0; i < voices.notes.size(); i++){
      if (voices.notes[i] == incoming) {
        voices.notes.erase(voices.notes.begin() + i);//Remove key
        break;
      }
    }
  }
  xSemaphoreGive(voices.mutex);
}


