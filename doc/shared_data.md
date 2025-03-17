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

As a `XXX_HandleTypeDef`, each handle interfaces with the hardware and as such does not need to be unique. Regardless, each one except `hadc1` is only used once, in the `setup()` function, which reduces the need for safe access methods. Indeed, the timer is the trigger for the DAC DMA outputs: they are all set up once, to not be modified again. Although the DAC buffer is continuously written to, its handle is never needed, and could be deallocated in a finished product, but is kept for code maintainability.
In contrast, the ADC handle will be repeatedly used in `scanKeysTask`, but in one-shot single value read mode. Hence, we also do not need a safe access method to interface it, especially since this ADC's sampling clock (when set in DMA mode) can be set at more than 1MHz, which is much more than what we sample it at.

## 6. Display Driver (u8g2)

The display driver is instantiated as an object, so it cannot be loaded / stored with atomic operations. However, as it is only used in the `updateDisplayTask`, no safe access methods are required.

## 7. Other Read-Only Data Structures

### 7.1 stepSizes

In order to store each unique step size for our phase accumulators, we store them in a `const` array of `uint32_t` that is filled at compile time with a function marked as `constexpr`. As it is read-only, we assume the `const` qualifier is enough to guarantee data safety.

### 7.2 Waveform LUTs

To increase read speed during the signal generation task, we have opted for C-like arrays instead of `std::vector` or `std::array`. These LUTs are only used in this task. As such, the `const` qualifier is enough to guarantee that if it is modified by accident, we will get an error. Because we need complex pre-computed values, such as in the sine wave LUT, we cannot give them an initial value on declaration easily. To work around this, we use `malloc()` to then calculate the values in the array and return a `const uint8_t*` to the head of the array. However, this does not make the values inside the array unmutable. We protect it using `mprotect()` in the function we use to allocate and calculate the values. These LUTs have to stay allocated and protected for the lifetime of the program, so we do not consider freeing the memory to be an issue. A typical allocation function looks like the function below.

```c
const uint8_t* generateSineWaveLUT(void)
{
    // Allocate Memory using malloc()
    const uint8_t* array = (uint8_t*) malloc(LUT_SIZE * sizeof(uint8_t));

    // Calculate the values in the array
    for(size_t i = 0; i < LUT_SIZE; i++)
    {
        array[i] = (uint8_t) ( (1 + sin( 2 * M_PI * i / LUT_SIZE )) * UINT8_MAX / 2 );
    }

    // Protect its memory
    if(mprotect(array, LUT_SIZE * sizeof(uint8_t), PROT_READ) != 0)
    {
        Serial.println("protect failed!");
        perror("mprotect failed");
        free(array);
        return NULL;
    }

    return array;
}
```
