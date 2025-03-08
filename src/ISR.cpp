#include <globals.h>
#include <ES_CAN.h>
#include <ISR.h>
#include <sig_gen.h>

void sampleISR(){
  static uint8_t waveforms[MAX_POLYPHONY] = {0};
  static uint32_t readCtr = 0;
  static uint8_t Vout = 0;
  static volatile uint8_t* dac_read_bufferLocal = dac_read_buffer;

  readCtr++;
  if(readCtr == DAC_BUFFER_SIZE)
  {
    readCtr = 0;
    writeBuffer1 = !writeBuffer1;

    HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);

    dac_read_buffer = writeBuffer1 ? dac_buffer2 : dac_buffer1;
    dac_write_buffer = writeBuffer1 ? dac_buffer1 : dac_buffer2; // should be fine without an atomic operation bc it's in the isr

    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*) dac_read_buffer, DAC_BUFFER_SIZE, DAC_ALIGN_8B_R);
    // HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*) dac_read_bufferLocal, DAC_BUFFER_SIZE, DAC_ALIGN_8B_R);

    xSemaphoreGiveFromISR(signalBufferSemaphore, NULL);
  }
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