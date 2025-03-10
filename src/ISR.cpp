#include <globals.h>
#include <ES_CAN.h>
#include <ISR.h>
#include <sig_gen.h>

void sampleISR(){
  static uint32_t readCtr = 0;
  /*
  Missed Interrupts going to be the absolute BANE of this system
  I tried making it as safe as possible, letting it catch as many errors as possible
  but modify if you think there's a better method
  */

  readCtr++;

  // // determine if this should be DAC_BUFFER_SIZE   OR   DAC_BUFFER_SIZE - 1
  // if(readCtr > DAC_BUFFER_SIZE - 1) // ConvCpltCallback
  // {
  //   readCtr = 0;
  //   writeBuffer1 = false;

  //   xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
  //   // Serial.println(writeBuffer1);
  // }
  // // order NEEDS to happen this way to prevent missed interrupts
  // else if(!writeBuffer1 && readCtr >= HALF_DAC_BUFFER_SIZE) // HalfConvCpltCallback
  // {
  //   writeBuffer1 = true;

  //   xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
  //   // Serial.println(writeBuffer1);
  // }

  if(readCtr == HALF_DAC_BUFFER_SIZE)
  {
    writeBuffer1 = true;
    xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
    // Serial.println("HALF CALLBACK");
  }
  else if(readCtr >= DAC_BUFFER_SIZE - 1)
  {
    readCtr = 0;
    writeBuffer1 = false;
    xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
    // Serial.println("FULL CALLBACK");
  }

}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
  writeBuffer1 = false;
  xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
  writeBuffer1 = true;
  xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
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