# Shared Data Structures & Safe Data Access

## 1. sysState
This is a struct of our own type ```sysState_t```, which contains critical information about our system, and is locked by a mutex.

```c
typedef struct __sysState_t{
    std::bitset<32> inputs;
    int Volume;
    bool mute = false;
    bool slave = false;
    uint8_t RX_Message[8];   
    int Octave = 0;
    bool joystickPress = false;
    int joystickHorizontalDirection = 0;
    int joystickVerticalDirection = 0;
    activityList_t activityList;
    waveform_t currentWaveform;
    SemaphoreHandle_t mutex;
} sysState_t;
```

## 2. writeBuffer1
This flag controls which half of the DAC's read buffer is being written to. As such, it is `volatile` and is loaded using an atomic operation into the local variable of the `signalGenTask` thread. Though timing analyses reveal we don't expect the flag to be written at any point during the thread's execution time, it is safer to load to a local variable using `__atomic_load()`

```c
// in globals.cpp
volatile uint8_t writeBuffer1 = false;

// in HAL_DAC_Conv*CpltCallback(DAC_HandleTypeDef* hdac)
writeBuffer1 = ... ;

// in signalGenTask(void *pvParameters)
static uint8_t writeBuffer1Local;
while(1)
{
    ...
    __atomic_load(&writeBuffer1, &writeBuffer1Local, __ATOMIC_RELAXED);
    ...
}

```

## 3. voices

## 4. msgInQ & msgOutQ

## 5. HAL Handles
As `XXX_HandleTypeDef`, each handle interfaces with the hardware and as such does not need to be unique. Regardless, each one except `hadc1` is only used once, in the `setup()` function, which reduces the need for safe access methods. Indeed, the timer is the trigger for the DAC DMA outputs: they are all set up once, to not be modified again. Although the DAC buffer is continuously written to, its handle is never needed, and could be deallocated in a finished product, but is kept for code maintainability.
In contrast, the ADC handle will be repeatedly used in `scanKeysTask`, but in one-shot single value read mode. Hence, we also do not need a safe access method to interface it, especially since this ADC's sampling clock (when set in DMA mode) can be set at more than 1MHz, which is much more than what we sample it at.

## 6. Display Driver (u8g2)
The display driver is instantiated as an object, so it cannot be loaded / stored with atomic operations. However, as it is only used in the `updateDisplayTask`, no safe access methods are required.

## X. Other Read-Only Data Structures