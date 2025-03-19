#include <globals.h>
#include <sig_gen.h>
#include <math.h>


const uint8_t shiftBits = 32 - LUT_BITS;

const uint8_t* generateSineWaveLUT(void)
{
    // Allocate Memory using malloc()
    uint8_t* array = (uint8_t*) malloc(LUT_SIZE * sizeof(uint8_t));

    // Calculate the values in the array
    for(size_t i = 0; i < LUT_SIZE; i++)
    {
        array[i] = (uint8_t) ( (1 + sin( 2 * M_PI * i / LUT_SIZE )) * UINT8_MAX / 2 );
    }

    const uint8_t* protected_arr = array;

    return protected_arr;
}

const uint8_t* generateSquareWaveLUT(void)
{
    // Allocate Memory using malloc()
    uint8_t* array = (uint8_t*) malloc(LUT_SIZE * sizeof(uint8_t));

    // Calculate the values in the array
    for(size_t i = 0; i < LUT_SIZE; i++)
    {
        array[i] = (i < LUT_SIZE / 2) ? 0 : UINT8_MAX;
    }

    const uint8_t* protected_arr = array;

    return protected_arr;
}

const uint8_t* generateTriangleWaveLUT(void)
{
    // Allocate Memory using malloc()
    uint8_t* array = (uint8_t*) malloc(LUT_SIZE * sizeof(uint8_t));

    // Calculate the values in the array
    for(size_t i = 0; i < LUT_SIZE; i++)
    {
        if (i < LUT_SIZE / 2) {
            array[i] = (uint8_t)((double)((i) / (LUT_SIZE / 2) * UINT8_MAX));
        } else {
            array[i] = (uint8_t)((double)(LUT_SIZE - i) / (LUT_SIZE / 2) * UINT8_MAX);
        }
    }

    const uint8_t* protected_arr = array;

    return protected_arr;
}

const uint8_t* generateSawtoothWaveLUT(void)
{
    // Allocate Memory using malloc()
    uint8_t* array = (uint8_t*) malloc(LUT_SIZE * sizeof(uint8_t));

    // Calculate the values in the array
    for(size_t i = 0; i < LUT_SIZE; i++)
    {
        array[i] = (uint8_t)((double) i / LUT_SIZE * UINT8_MAX);
    }

    const uint8_t* protected_arr = array;

    return protected_arr;
}

const uint8_t* sineWave = generateSineWaveLUT();

const uint8_t* squareWave = generateSquareWaveLUT();

const uint8_t* triangleWave = generateTriangleWaveLUT();

const uint8_t* sawtoothWave = generateSawtoothWaveLUT();

void signalGenTask(void *pvParameters) {
    static waveform_t currentWaveformLocal;
    static bool writeBuffer1Local = false;
    uint8_t Vout;
    int volumeLocal;
    bool muteLocal;

    #ifdef GET_MASS_SIG_GEN
    uint32_t prevTime = micros();
    #endif

    while (1) {
        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        __atomic_load(&sysState.currentWaveform, &currentWaveformLocal, __ATOMIC_RELAXED);
        __atomic_load(&sysState.Volume, &volumeLocal, __ATOMIC_RELAXED);
        __atomic_load(&sysState.mute, &muteLocal, __ATOMIC_RELAXED);
        xSemaphoreGive(sysState.mutex);

        xSemaphoreTake(signalBufferSemaphore, portMAX_DELAY);

        writeBuffer1Local = writeBuffer1; // atomic operation

        dac_write_HEAD = writeBuffer1Local ? dac_buffer : &dac_buffer[HALF_DAC_BUFFER_SIZE];

        if(muteLocal) continue;

        fillBuffer(currentWaveformLocal, dac_write_HEAD, HALF_DAC_BUFFER_SIZE,volumeLocal);

        #ifdef GET_MASS_SIG_GEN
        monitorStackSize();
        #endif
    }
}



inline void fillBuffer(waveform_t wave, volatile uint8_t buffer[], uint32_t size, int volume) {
    static uint8_t voiceIndex;
    static uint32_t waveIndex;
    if (uxSemaphoreGetCount(voices.mutex) == 0){
        Serial.println("Voices locked (fillBuffer)");
    }
    xSemaphoreTake(voices.mutex, portMAX_DELAY);

    // Select the lookup table based on the waveform type.
    const uint8_t *waveformLUT;
    switch (wave) {
        case SAWTOOTH:
            waveformLUT = sawtoothWave;
            break;
        case SINE:
            waveformLUT = sineWave;
            break;
        case SQUARE:
            waveformLUT = squareWave;
            break;
        case TRIANGLE:
            waveformLUT = triangleWave;
            break;
        default:
            waveformLUT = sineWave;
            break;
    }

    // For each sample in the buffer...
    for (uint32_t i = 0; i < size; i++) {
        uint32_t sampleSum = 0;
        for (auto note : voices.notes){//For each note currently played
            voiceIndex = note.first * 12 + note.second;//Get index
            voices.voices_array[voiceIndex].phaseAcc += voices.voices_array[voiceIndex].phaseInc;//Increment phase accum
            waveIndex = voices.voices_array[voiceIndex].phaseAcc >> shiftBits;//Get wave index
            uint8_t sample = waveformLUT[waveIndex];
            sampleSum += sample >> (9 - volume);//Add to sample
        }
        // Write the summed sample to the output buffer.
        saturate2uint8_t(sampleSum,0,UINT8_MAX);
        buffer[i] = static_cast<uint8_t>(sampleSum);
    }
    xSemaphoreGive(voices.mutex);
}


inline void saturate2uint8_t(volatile uint32_t & x, int min, int max)
{
    x = (x < min) ? (min) : ( (x > max) ? max : x );
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