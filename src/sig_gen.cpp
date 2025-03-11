#include <globals.h>
#include <sig_gen.h>
// #include <stm32l432xx.h>
// #include <stm32l4xx_hal_dac.h>

/*
Think about improving the safety of this code
I want to load / store things atomically or with mutexes but icl
the route might be to put the notesPlayed in a struct with a semaphore
and go from there
*/

void signalGenTask(void *pvParameters) {
    static waveform_t currentWaveformLocal;
    static bool writeBuffer1Local = false;
    uint8_t Vout;
    int volumeLocal;
    bool muteLocal;

    uint32_t current_freq = 0;
    int numkeys;

    while (1) {
        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        __atomic_load(&sysState.currentWaveform, &currentWaveformLocal, __ATOMIC_RELAXED);
        __atomic_load(&sysState.Volume, &volumeLocal, __ATOMIC_RELAXED);
        __atomic_load(&sysState.mute, &muteLocal, __ATOMIC_RELAXED);
        xSemaphoreGive(sysState.mutex);

        xSemaphoreTake(signalBufferSemaphore, portMAX_DELAY);

        currentWaveformLocal = SINE; // BAD modify
        volumeLocal = 7; // BAD modify

        __atomic_load(&writeBuffer1, &writeBuffer1Local, __ATOMIC_RELAXED);

        dac_write_HEAD = writeBuffer1Local ? dac_buffer : &dac_buffer[HALF_DAC_BUFFER_SIZE];

        memset((uint8_t*) dac_write_HEAD, (uint8_t) 0, HALF_DAC_BUFFER_SIZE); // clear buffer much faster
        // for(size_t i = 0; i < HALF_DAC_BUFFER_SIZE; i++) dac_write_HEAD[i] = 0;

        if(muteLocal) continue;
        
        numkeys = numKeysPressed();

        for(size_t i = 0; i < 12; i++)
        {
            if(notesPlayed[i].empty()) continue;
            
            for(size_t j = 0; j < notesPlayed[i].size(); j++)
            {
                // uint32_t freq = baseFreqs[i] << notesPlayed[i][j]; // bit shift by -4 < shift < 4
                current_freq = (notesPlayed[i][j] >= 4) ? 
                                (baseFreqs[i] << (notesPlayed[i][j] - 4)) : 
                                (baseFreqs[i] >> abs(notesPlayed[i][j] - 4));

                fillBuffer(currentWaveformLocal, dac_write_HEAD, HALF_DAC_BUFFER_SIZE, current_freq, numkeys, volumeLocal);
            }
        }
    }
}

void fillBuffer(waveform_t wave, volatile uint8_t buffer[], uint32_t size, uint32_t frequency, int numkeys, int volume)
{
    static double normalised_ang_freq = 2 * M_PI * 1 / F_SAMPLE_TIMER;
    static uint8_t Vout;

    // Serial.println("fillBuffer");

    normalised_ang_freq = 2 * M_PI * frequency / F_SAMPLE_TIMER;

    for (size_t i = 0; i < size; i++) 
    {
        switch(wave)
        {
            case SINE: // should be correct
            {
                Vout = (uint8_t) (127 * sin(normalised_ang_freq * i) + 128);
                break;
            }
                
            case SAWTOOTH: // fix, is incorrect atm
            {
                Vout = (255.0f * i / size);
                break;
            }

            case SQUARE: // buffer1 is positive part, buffer2 is negative part // fix, is incorrect atm
            {
                Vout = (i < (size / 2) ? 255 : 0);
                break;
            }

            case TRIANGLE: // buffer 1 counts up, buffer 2 counts down // fix, is incorrect atm
            {
                if(i < size / 2)
                {
                    Vout = (255.0f * i / (size / 2));
                }
                else
                {
                    Vout = (255.0f * (size - i) / (size / 2));
                }
                break;
            }

            default:
            {
                Vout = 0;
                break;
            }
        }

        buffer[i] += Vout / numkeys; // += is critical to add multiple sine waves
        buffer[i] = buffer[i] >> (8 - volume);
        // saturate2uint8_t(buffer[i], 0, UINT8_MAX);
    }
    // for(int i = 0; i < DAC_BUFFER_SIZE; i++)
    // {
    //     Serial.print(dac_buffer[i]);
    //     Serial.print(" ");
    // }
    // Serial.print(numkeys);
    // Serial.println("");
}

inline void saturate2uint8_t(volatile uint32_t & x, int min, int max)
{
    x = (x < min) ? (min) : ( (x > max) ? max : x );
}

int numKeysPressed()
{
    int cnt = 0;
    for(int i = 0; i < 12; i++) cnt += notesPlayed[i].size();
    return cnt;
}


uint32_t Set_TIMx_Frequency(TIM_HandleTypeDef* htim, uint32_t freq_desired)
{
    // Calculate the desired timer update frequency.
    uint32_t f_update = freq_desired;

    // Calculate the ideal division factor.
    uint32_t idealDiv = HAL_RCC_GetPCLK1Freq() / f_update;

    uint32_t bestPSC = 0, bestARR = 0, bestError = 0xFFFFFFFF, actualFreq = 0;

    // Iterate over possible prescaler values.
    // Here we search PSC values from 0 to a reasonable limit (e.g., 1024) to find valid values.
    for (uint32_t psc = 0; psc < 1024; psc++)
    {
        uint32_t divider = psc + 1;
        uint32_t arr_plus1 = idealDiv / divider;
        if (arr_plus1 == 0 || arr_plus1 > 65536)  // ARR must be 0..65535 (i.e. ARR+1 <= 65536)
            continue;

        uint32_t arr = arr_plus1 - 1;
        // Calculate the actual update frequency for this configuration.
        actualFreq = HAL_RCC_GetPCLK1Freq() / ((psc + 1) * (arr + 1));
        // Compute the absolute error.
        uint32_t error = (f_update > actualFreq) ? (f_update - actualFreq) : (actualFreq - f_update);
        if (error < bestError)
        {
            bestError = error;
            bestPSC = psc;
            bestARR = arr;
        }
        if (error == 0)
            break;  // Perfect match found.
    }

    // Set the timer registers:
    __HAL_TIM_SET_PRESCALER(htim, bestPSC);
    __HAL_TIM_SET_AUTORELOAD(htim, bestARR);
    // Force an update event so that new PSC/ARR values are loaded immediately.
    HAL_TIM_GenerateEvent(htim, TIM_EVENTSOURCE_UPDATE);

    // Return the actual timer update frequency (f_update_actual = F_TIMER_CLOCK / ((PSC+1)*(ARR+1)) )
    return HAL_RCC_GetPCLK1Freq() / ((bestPSC + 1) * (bestARR + 1));
}