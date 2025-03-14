#include <globals.h>
#include <sig_gen.h>
#include <math.h>




// Generate a sine wave table with 256 entries scaled to [0, 255]
constexpr std::array<uint8_t, SINE_LUT_SIZE> generateSineWave() {
    std::array<uint8_t, SINE_LUT_SIZE> table = {};
    for (size_t i = 0; i < table.size(); i++) {
        // Convert the index to an angle (radians)
        double angle = (2.0 * M_PI * i) / table.size();
        // Compute sine value, then map from [-1,1] to [0,255]
        table[i] = static_cast<uint8_t>((std::sin(angle) + 1.0) * ((double) 256 / 2.0));
    }
    return table;
}

constexpr std::array<uint8_t, SINE_LUT_SIZE> generateSquareWave() {
    std::array<uint8_t, SINE_LUT_SIZE> table = {};
    for (size_t i = 0; i < table.size(); i++) {
        // For the first half, output 0; for the second half, output 255.
        table[i] = (i < table.size() / 2) ? 0 : 255;
    }
    return table;
}

constexpr std::array<uint8_t, SINE_LUT_SIZE> generateTriangleWave() {
    std::array<uint8_t, SINE_LUT_SIZE> table = {};
    size_t half = table.size() / 2;
    for (size_t i = 0; i < table.size(); i++) {
        if (i < half) {
            // Linear ramp up: i from 0 to half-1 maps to 0 to 255.
            double value = static_cast<double>(i) / (half - 1) * 255.0;
            table[i] = static_cast<uint8_t>(value);
        } else {
            // Linear ramp down: i from half to table.size()-1 maps to 255 down to 0.
            double value = static_cast<double>(table.size() - 1 - i) / (half - 1) * 255.0;
            table[i] = static_cast<uint8_t>(value);
        }
    }
    return table;
}

constexpr std::array<uint8_t, SINE_LUT_SIZE> generateSawtoothWave() {
    std::array<uint8_t, SINE_LUT_SIZE> table = {};
    for (size_t i = 0; i < table.size(); i++) {
         double value = static_cast<double>(i) / (table.size() - 1) * 255.0;
         table[i] = static_cast<uint8_t>(value);
    }
    return table;
}



// Precomputed sine wave lookup table available at compile time
const auto sineWave = generateSineWave();

const auto squareWave = generateSquareWave();

const auto triangleWave = generateTriangleWave();

const auto sawtoothWave = generateSawtoothWave();

const uint8_t shiftBits = 32 - LUT_BITS;

uint8_t sinTable[SINE_LUT_SIZE];


// uint8_t sineWave[SINE_LUT_SIZE];

// void fillSineWaveBufer(void)
// {
//     for(size_t i = 0; i < SINE_LUT_SIZE; i++)
//     {
//         sineWave[i] = (uint8_t) (128 + 127 * sin( 2 * M_PI * i / SINE_LUT_SIZE ));
//     }
// }

void signalGenTask(void *pvParameters) {
    for (size_t i = 0; i < SINE_LUT_SIZE; i++) {
        // Convert the index to an angle (radians)
        double angle = (2.0 * M_PI * i) / SINE_LUT_SIZE;
        // Compute sine value, then map from [-1,1] to [0,255]
        sinTable[i] = (uint8_t)((sin(angle) + 1.0) * ((double) 256 / 2.0));
    }
    static waveform_t currentWaveformLocal;
    static bool writeBuffer1Local = false;
    uint8_t Vout;
    int volumeLocal;
    bool muteLocal;


    // fillSineWaveBufer();

    while (1) {
        //Serial.println("SigGen");
        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        __atomic_load(&sysState.currentWaveform, &currentWaveformLocal, __ATOMIC_RELAXED);
        __atomic_load(&sysState.Volume, &volumeLocal, __ATOMIC_RELAXED);
        __atomic_load(&sysState.mute, &muteLocal, __ATOMIC_RELAXED);
        xSemaphoreGive(sysState.mutex);
        // if (uxSemaphoreGetCount(signalBufferSemaphore) == 0){
        //     Serial.println("Mutex Locked (Written and waiting)");
        // }
        // else{
        //     Serial.println("Mutex Available (Waitng for write)");
        // }
        xSemaphoreTake(signalBufferSemaphore, portMAX_DELAY);

        __atomic_load(&writeBuffer1, &writeBuffer1Local, __ATOMIC_RELAXED);

        dac_write_HEAD = writeBuffer1 ? dac_buffer : &dac_buffer[HALF_DAC_BUFFER_SIZE];

        // memset((uint8_t*) dac_write_HEAD, (uint8_t) 0, HALF_DAC_BUFFER_SIZE); // clear buffer much faster
        // for(size_t i = 0; i < HALF_DAC_BUFFER_SIZE; i++) dac_write_HEAD[i] = 0;

        if(muteLocal) continue;

        fillBuffer(currentWaveformLocal, dac_write_HEAD, HALF_DAC_BUFFER_SIZE,volumeLocal);

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
    uint8_t *waveformLUT;
    switch (wave) {
        case SAWTOOTH:
            // waveformLUT = sawtoothWave;
            break;
        case SINE:
            waveformLUT = sinTable;
            break;
        case SQUARE:
            // waveformLUT = squareWave;
            break;
        case TRIANGLE:
            // waveformLUT = triangleWave;
            break;
        default:
            // waveformLUT = sineWave;
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
            sampleSum += sample >> (8 - volume);//Add to sample
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