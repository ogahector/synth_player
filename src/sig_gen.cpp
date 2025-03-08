#include <globals.h>
#include <sig_gen.h>
// #include <stm32l432xx.h>
// #include <stm32l4xx_hal_dac.h>


volatile bool writeBuffer1;

volatile uint8_t dac_buffer1[DAC_BUFFER_SIZE];
volatile uint8_t dac_buffer2[DAC_BUFFER_SIZE];
volatile uint8_t* dac_write_buffer = dac_buffer1;
volatile uint8_t* dac_read_buffer = dac_buffer2;

void signalGenTask(void *pvParameters) {
    static waveform_t currentWaveformLocal;
    uint8_t Vout;

    uint32_t current_freq = 0;

    while (1) {
        // xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        // Serial.println("signalGenTask mutex taken");

        // __atomic_load(&sysState.currentWaveform, &currentWaveformLocal, __ATOMIC_RELAXED);

        // xSemaphoreGive(sysState.mutex);
        // Serial.println("signalGenTask mutex given");

        xSemaphoreTake(signalBufferSemaphore, portMAX_DELAY);
        Serial.println("signalBufferSemaphore taken");

        currentWaveformLocal = SINE; // BAD modify

        dac_write_buffer = writeBuffer1 ? dac_buffer1 : dac_buffer2; // redundant from sampleISR

        for(size_t i = 0; i < 12; i++)
        {
            if(notesPlayed[i].empty()) continue;

            for(size_t j = 0; j < notesPlayed[i].size(); j++)
            {
                Serial.println("signalGenTask 2nd for loop");
                // uint32_t freq = baseFreqs[i] << notesPlayed[i][j]; // bit shift by -4 < shift < 4
                if(notesPlayed[i][j] >= 4)
                {
                    current_freq = baseFreqs[i] << (notesPlayed[i][j] - 4);
                }
                else
                {
                    current_freq = baseFreqs[i] >> abs(notesPlayed[i][j] - 4);
                }

                fillBuffer(currentWaveformLocal, dac_write_buffer, DAC_BUFFER_SIZE, current_freq);

                for(int k = 0; k < DAC_BUFFER_SIZE; k++)
                {
                    Serial.print(dac_write_buffer[k]);
                    Serial.print(" ");
                }
            }
        }
    }
}

void fillBuffer(waveform_t wave, volatile uint8_t buffer[], uint32_t size, uint32_t frequency)
{
    static double normalised_ang_freq = 2 * M_PI * 1 / F_SAMPLE_TIMER;
    static uint8_t Vout;

    Serial.println("fillBuffer");

    normalised_ang_freq = 2 * M_PI * frequency / F_SAMPLE_TIMER;

    for (size_t i = 0; i < size; i++) 
    {
        switch(wave)
        {
            case SINE: // should be correct
                Vout = (uint8_t) (128 * sin(normalised_ang_freq * i) + 128);
                break;

            case SAWTOOTH: // fix, is incorrect atm
                Vout = (uint8_t) (255.0f * i / size);
                break;

            case SQUARE: // buffer1 is positive part, buffer2 is negative part // fix, is incorrect atm
                Vout = (uint8_t) (i < (size / 2) ? 255 : 0);
                break;

            case TRIANGLE: // buffer 1 counts up, buffer 2 counts down // fix, is incorrect atm
                if(i < size / 2)
                {
                    Vout = (uint8_t) (255.0f * i / (size / 2));
                }
                else
                {
                    Vout = (uint8_t) (255.0f * (size - i) / (size / 2));
                }
                break;

            default:
                Vout = 0;
                break;
        }

        buffer[i] += Vout; // += is critical to add multiple sine waves
    }

    for(size_t i = 0; i < DAC_BUFFER_SIZE; i++)
    {
        Serial.print(buffer[i]);
        Serial.print(" ");
    }
}