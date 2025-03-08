#pragma once
#include <globals.h>

void signalGenTask(void *pvParameters);

void fillBuffer(waveform_t wave, volatile uint32_t buffer[], uint32_t start, uint32_t size, uint32_t frequency);