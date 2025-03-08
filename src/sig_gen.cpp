#include <globals.h>
#include <sig_gen.h>
// #include <stm32l432xx.h>
// #include <stm32l4xx_hal_dac.h>



void signalGenTask(void *pvParameters) {
    static waveform_t currentWaveformLocal;
    uint8_t Vout;

    uint32_t current_freq = 0;

    while (1) {
        // xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        // __atomic_load(&sysState.currentWaveform, &currentWaveformLocal, __ATOMIC_RELAXED);
        // xSemaphoreGive(sysState.mutex);

        xSemaphoreTake(signalBufferSemaphore, portMAX_DELAY);
        Serial.print("entered signalGenTask"); Serial.println(writeBuffer1);

        currentWaveformLocal = SINE; // BAD modify


        size_t start = writeBuffer1 ? 0 : HALF_DAC_BUFFER_SIZE;
        dac_write_HEAD = &dac_buffer[start];

        memset((uint8_t*) dac_write_HEAD, 0, HALF_DAC_BUFFER_SIZE); // clear buffer

        for(size_t i = 0; i < 12; i++)
        {
            if(notesPlayed[i].empty()) continue;
            
            for(size_t j = 0; j < notesPlayed[i].size(); j++)
            {
                // uint32_t freq = baseFreqs[i] << notesPlayed[i][j]; // bit shift by -4 < shift < 4
                if(notesPlayed[i][j] >= 4)
                {
                    current_freq = baseFreqs[i] << (notesPlayed[i][j] - 4);
                }
                else
                {
                    current_freq = baseFreqs[i] >> abs(notesPlayed[i][j] - 4);
                }

                fillBuffer(currentWaveformLocal, dac_write_HEAD, start, HALF_DAC_BUFFER_SIZE, current_freq);
            }
        }
    }
}

void fillBuffer(waveform_t wave, volatile uint32_t buffer[], uint32_t start, uint32_t size, uint32_t frequency)
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
                Vout = (uint8_t) (128 * sin(normalised_ang_freq * i) + 128);
                break;
            }
                
            case SAWTOOTH: // fix, is incorrect atm
            {
                Vout = (uint8_t) (255.0f * i / size);
                break;
            }

            case SQUARE: // buffer1 is positive part, buffer2 is negative part // fix, is incorrect atm
            {
                Vout = (uint8_t) (i < (size / 2) ? 255 : 0);
                break;
            }

            case TRIANGLE: // buffer 1 counts up, buffer 2 counts down // fix, is incorrect atm
            {
                if(i < size / 2)
                {
                    Vout = (uint8_t) (255.0f * i / (size / 2));
                }
                else
                {
                    Vout = (uint8_t) (255.0f * (size - i) / (size / 2));
                }
                break;
            }

            default:
            {
                Vout = 0;
                break;
            }
        }

        buffer[start + i] += Vout; // += is critical to add multiple sine waves
    }
}