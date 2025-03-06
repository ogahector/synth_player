#include <globals.h>
#include <ES_CAN.h>
#include <ISR.h>

void sampleISR(){
    static uint32_t phaseAcc[MAX_POLYPHONY] = {0};
    static int32_t Vout = 0;
    uint8_t counter = 0;//Counts the number of active notes
    for (uint8_t i = 0; i < 12; i++) {//Iterates through all keys
      if (activeStepSizes[i] != 0 && counter < MAX_POLYPHONY){
        phaseAcc[counter] += activeStepSizes[i];//Phase accumulator has channels now
        Vout += ((phaseAcc[counter] >> 24) - 128);
        counter++;
      }
    }
    //NOTE : with above, the first keys pressed from left to right will take preference when there are more than 4 keys pressed
    //This is because the counter will not increment past 4, and the phaseAcc will not be updated for the other keys
    //This is a limitation of the current implementation, may want to change this in the future
    int volume;
    bool mute;
    __atomic_load(&sysState.Volume, &volume, __ATOMIC_RELAXED);
    __atomic_load(&sysState.mute, &mute, __ATOMIC_RELAXED);
    if (mute) Vout = -128;
    else{
      Vout = Vout >> (8 - volume);
      Vout = Vout / max(1, (int)counter);//Reduces output slightly with extra keys
    }
    analogWrite(OUTR_PIN, Vout + 128);
}
  
  
void CAN_RX_ISR (void) {//Recieving CAN ISR
    uint8_t RX_Message_ISR[8];
    uint32_t ID;
    CAN_RX(ID, RX_Message_ISR);
    xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
}
  
void CAN_TX_ISR (void) {//Transmitting can ISR
    xSemaphoreGiveFromISR(CAN_TX_Semaphore, NULL);
}