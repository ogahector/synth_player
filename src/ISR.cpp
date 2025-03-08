#include <globals.h>
#include <ES_CAN.h>
#include <ISR.h>
#include <sig_gen.h>

void sampleISR(){
  static uint32_t readCtr = 0;

  readCtr++;
  // if(readCtr == DAC_BUFFER_SIZE)
  // {
  //   readCtr = 0;
  //   writeBuffer1 = !writeBuffer1;

  //   dac_read_buffer = writeBuffer1 ? dac_buffer2 : dac_buffer1;
  //   dac_write_buffer = writeBuffer1 ? dac_buffer1 : dac_buffer2; // should be fine without an atomic operation bc it's in the isr

  //   xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
  // }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  writeBuffer1 = false;
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
  writeBuffer1 = true;
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