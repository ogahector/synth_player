# Shared Data Structures & Safe Data Access

## 1. sysState

This is a struct of our own type ```sysState_t```, which contains critical information about our system, and is locked by a mutex. We considered removing some variables from sysState and instead having them as standalone global `std::atomic<>` variables, particularly flags and enums such as `mute`, `slave`, `currentWaveform`, and `activityList`, but ultimately we require the entire sysState to be in sync with itself: the volume or the master/slave configuration shouldn't change independently of other factors.
Additionally, the overhead we do gain from the few variables we can load / store using atomic operations is minimal, as they often have to be loaded in conjunction with other variables that do not have the same potential for atomic operations. A key example of this is in the Decode Task, in which the `mute`, and `slave` states are updates, which could be done atomically, but the `RX_Message` cannot be loaded as such and needs to be in sync with the rest of `sysState`. Therefore, we have chosen to not have the `sysState` flags as atomic variables to leverage readability and code maintanability.

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
    activityList_t activityList; //An enum containing the current activity, e.g. MENU, HOME, RECORDING...
    waveform_t currentWaveform; //An enum containing the current waveform, e.g. SINE, SQUARE, SAWTOOTH...
    SemaphoreHandle_t mutex;
} sysState_t;
```

## 2. writeBuffer1

This flag controls which half of the DAC's read buffer is being written to. As such, it is `volatile` and is as an atomic variable of type `std::atomic_bool`. Though timing analyses reveal we don't expect the flag to be written at any point during the thread's execution time, it is safer to load to a local variable using the built-in atomic operations into the local variable of the `signalGenTask` thread instead.

```c
// in globals.cpp
volatile std::atomic_bool writeBuffer1 = false;

// in HAL_DAC_Conv*CpltCallback(DAC_HandleTypeDef* hdac)
writeBuffer1 = ... ;

// in signalGenTask(void *pvParameters)
static bool writeBuffer1Local;
while(1)
{
    ...
    writeBuffer1Local = writeBuffer1;
    ...
}

```

## 3. voices

In order to more elegantly store the various phase increments and phase accumulators we use for DAC DMA indexing, we have made two struct datatypes, `voice_t` and `voices_t`, defined as below:

```c
typedef struct __voice_t{
    uint32_t phaseAcc = 0;
    uint32_t phaseInc = 0;
} voice_t;

typedef struct __voices_t{
    std::vector<std::pair<uint8_t, uint8_t>> notes; // Stores octave and key/note played
    voice_t voices_array[108]; //Index by octave * 12 + key/note played
    SemaphoreHandle_t mutex;
} voices_t;
```

Each voice is stored in a LUT, in order to avoid computing each value on the fly. The phase increments are calculated for each voice on startup, and they simply increment the accumulator if the chosen note is present in the notes vector. Additionally, this global struct is locked by a mutex, as it is accessed continuously by both the decoding task and the signal generation task.

## 4. record
The recording struct is defined to handle the flags for recording, as well as controlling the active tracks.
```c
typedef struct __record_t{
    bool recording = false;
    bool playback = false;
    uint8_t active_tracks = 0b0000; //Active tracks with 1 hot encoding.
    uint8_t current_track = 0; //Track you are currently recording / playing
    SemaphoreHandle_t mutex;
} record_t;
```
The `active_tracks` is used to control how many of the four tracks are currently active, using one hot encoding. The `current_track` is used to determine which track you are currently performing actions on. This is also protected by a mutex, as it's accessed from both the display update task, and the recording task. 

## 5. msgInQ & msgOutQ
<!-- Really not sure what to put here -->
FreeRTOS queues (or any RTOS queues) are designed to be accessed safely from both tasks and ISRs. When a CAN message is received by an ISR, the message can be placed into the queue in an atomic operation, making it a thread-safe embedded practice.

The queue implementation inherently manages mutual exclusion. When a task reads (dequeues) a message from the queue, it does so in a controlled, thread-safe manner. There’s no need for additional mutexes or semaphores to protect the message data.

## 6. HAL Handles

As a hardware handles, each handle interfaces with the hardware and as such does not need to be unique. Indeed, the timer is the trigger for the DAC DMA outputs: they are all set up once, to not be modified again. Although the DAC buffer is continuously written to, its handle is never needed, and could be deallocated in a finished product, but is kept for code maintainability.

## 7. Display Driver (u8g2)

The display driver is instantiated as an object, so it cannot be loaded / stored with atomic operations. However, as it is only used in the `updateDisplayTask`, no safe access methods are required.

## 8. Other Read-Only Data Structures

### 8.1 stepSizes

In order to store each unique step size for our phase accumulators, we store them in a `const` array of `uint32_t` that is filled at compile time with a function marked as `constexpr`. As it is read-only, we assume the `const` qualifier is enough to guarantee data safety.

### 8.2 Waveform LUTs

To increase read speed during the signal generation task, we have opted for C-like arrays instead of `std::vector` or `std::array`. These LUTs are only used in this task. As such, the `const` qualifier is enough to guarantee that if it is modified by accident, we will get an error. Because we need complex pre-computed values, such as in the sine wave LUT, we cannot give them an initial value on declaration easily. To work around this, we use `malloc()` to calculate the values in the array and return a `const uint8_t*` to the head of the array. Modern compilers (including ours) will prevent any data from being written to an address given by a `const` pointer, and subsequently all other cells related to this pointer. These LUTs have to stay allocated and protected for the lifetime of the program, so we do not consider freeing the memory to be an issue. A typical allocation function looks like the function below.

```c
const uint8_t* generateSineWaveLUT(void)
{
    // Allocate Memory using malloc()
    uint8_t* array = (uint8_t*) malloc(LUT_SIZE * sizeof(uint8_t));

    // Calculate the values in the array
    for(size_t i = 0; i < LUT_SIZE; i++)
    {
        array[i] = (uint8_t) ( (1 + sin( 2 * M_PI * i / LUT_SIZE )) * UINT8_MAX / 2 );
    }

    const uint8_t* protected_array = array;

    return protected_array;
}
```

## 9. Stack Size Allocation

To analyse and optimise the stack size allocation, we used the `uxTaskGetStackHighWaterMark(NULL)` to determine the High Water Mark of the current thread's stack size. We have decided to leave 25% extra words of overhead per thread in case of undefined behavior, but the current stack sizes we allocate have had an extensive series of testing which included:
<!-- see about including these tests idk how useful they are -->
- 3 Keyboards Connected to each other
  - Of which 1 Master (Player) and 2 Slaves (Senders)
- 20 Keys Pressed at Once
- Changing Waveform Types
- Menu, Home, Waveform, Recording, and Doom menu navigation

| Task | Max Used Stack Size (words) | Allocated Stack Size (words) |
| ---- | ------------------- | -------------------- |
| Scan Keys | 82 | 103 |
| Decode | 46 | 58 |
| Record | XX | XX |
| Signal Generator | 44 | 55 |
| Transmit | XX | XX |
| Display Update | 167 | 209 |
<!-- STILL NEED TO GET VALUES FOR TRANSMIT AND RECORD -->

## 10. Full system graph
Below is an image showing a high level depiction of our system, and how it handles shared data.
![Full System Diagram](/Images/full_graph.png)