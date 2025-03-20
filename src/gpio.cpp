#include <gpio.h>
#include <globals.h>

extern ADC_HandleTypeDef hadc1;

int analogReadHAL(uint32_t pin)
{
    static uint32_t prevchannel;
    static ADC_ChannelConfTypeDef sConfig = {
        .Channel = PA0, // by default
        .Rank = ADC_REGULAR_RANK_1,
        // should be plenty and about 12.06us
        .SamplingTime = ADC_SAMPLETIME_247CYCLES_5
    };

    if(pin != PA0 && pin != PA1)
    {
        return -1; // 0xFFFF_FFFF
    }

    uint32_t channel;
    if(pin == PA0)
    {
        channel = ADC_CHANNEL_5;
    }
    else if(pin == PA1)
    {
        channel = ADC_CHANNEL_6;
    }
    else
    {
        Serial.println("What's going on??");
    }

    if(prevchannel != channel)
    {
        // deinit and reinit adc channel
        sConfig.Channel = channel;
        if(HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
        {
            while (1) Serial.println("ERROR REINITIALISING ADC IN ANREAD");
        }
        prevchannel = channel;
        // Serial.println("Good Config");
    }

    HAL_ADC_Start(&hadc1);

    // while(!adcConvDone) { Serial.println("Waiting for IT Flag"); }


    while (!(__HAL_ADC_GET_FLAG(&hadc1, ADC_FLAG_EOC))) 
    {
        // Wait for EOC
    }

    int adc_val = (int) HAL_ADC_GetValue(&hadc1);

    // if(pin == JOYX_PIN) Serial.println(adc_val >> 2);

    HAL_ADC_Stop(&hadc1);

    return adc_val >> 2 ; // 12 bits -> 10 bits
}

