#pragma once
#include <globals.h>



void signalGenTask(void *pvParameters);

inline void fillBuffer(waveform_t wave, volatile uint8_t buffer[], uint32_t size, int volume);

float cubicInterpolate(float p0, float p1, float p2, float p3, float x);

inline void saturate2uint8_t(volatile uint32_t & x, int min, int max);

uint32_t Set_TIMx_Frequency(TIM_HandleTypeDef* htim, uint32_t freq_desired);