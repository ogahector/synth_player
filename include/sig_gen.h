#pragma once
#include <globals.h>
#include <map>
#include <vector>


#define DAC_BUFFER_SIZE 100 // effective size will be 2x
#define HALF_DAC_BUFFER_SIZE (DAC_BUFFER_SIZE / 2)

#define NUM_WAVES 4

extern volatile bool writeBuffer1;
extern volatile uint8_t dac_buffer[DAC_BUFFER_SIZE];
extern volatile uint8_t* dac_write_HEAD;

void signalGenTask(void *pvParameters);

void fillBuffer(waveform_t wave, volatile uint8_t buffer[], uint32_t start, uint32_t size, uint32_t frequency);