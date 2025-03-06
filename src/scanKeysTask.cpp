#include <Arduino.h>
#include <bitset>
#include <STM32FreeRTOS.h>
#include <knob.h>
#include <globals.h>
#include <scanKeysTask.h>



std::bitset<4> readCols(){
    std::bitset<4> result;
    result[0] = digitalRead(C0_PIN);
    result[1] = digitalRead(C1_PIN); 
    result[2] = digitalRead(C2_PIN);
    result[3] = digitalRead(C3_PIN);
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
  const TickType_t xFrequency = 20/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();

  Knob K3 = Knob(0,8);//Volume Knob
  Knob K2 = Knob(-4,4);//Octave Knob

  bool muteReleased = true;
  bool slaveReleased = true;
  bool prevDoomButton = false;
  bool shootButton = false;

  std::bitset<32> previousInputs;
  std::bitset<4> cols;

  uint8_t TX_Message[8] = {0};//Message sent over CAN

  while(1){
    vTaskDelayUntil( &xLastWakeTime, xFrequency );
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
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
        xQueueSend( msgOutQ, TX_Message, portMAX_DELAY);//Sends via CAN
      }
    }

    if (!sysState.slave) 
      sysState.Volume = K3.update(sysState.inputs[12], sysState.inputs[13]); //Volume adjustment
    sysState.Octave = K2.update(sysState.inputs[14], sysState.inputs[15]); //Octave Adjustment
    
    //Toggles mute (Knob 3 Press)
    if(!sysState.inputs[21] && muteReleased) {
      muteReleased = false;
      sysState.mute = !sysState.mute;
    }
    else if (sysState.inputs[21]) muteReleased = true;

    //Toggles slave (Knob 2 Press)
    if(!sysState.inputs[20] && slaveReleased) {
      slaveReleased = false;
      sysState.slave = !sysState.slave;
    }
    else if (sysState.inputs[20]) slaveReleased = true;

    //Toggles DOOM (Knob 0 Press)
    if(!sysState.inputs[24] && prevDoomButton) {
      prevDoomButton = false;
      sysState.doomMode = !sysState.doomMode;
      if (sysState.doomMode) {
        doomLoadingShown = false;  // Reset flag when entering doom mode
      }
    }
    else if (sysState.inputs[24]) prevDoomButton = true;

    if(!sysState.inputs[22]) {
      sysState.joystickPress = true;
    }
    else if (sysState.inputs[22]) sysState.joystickPress = false;;

    xSemaphoreGive(sysState.mutex);
  }
}