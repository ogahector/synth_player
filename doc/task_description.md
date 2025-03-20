# Task Identification & Description

All the tasks implemented on this board are:

1. [Scan Keys Thread](#scan-keys-thread)
2. [Display Update Thread](#display-update-thread)
3. [CAN Decode Thread](#can-decode-thread)
4. [CAN Encode Thread](#can-transmit-thread)
5. [Wave Generation Task](#custom-wave-generation-task)

## Scan Keys Thread

The goal of this task is to repeatedly scan for key presses, on the keyboard, the knobs, and the joystick.

To optimise this task, the system wide state machine determines which keys should be scanned for each state. The master knob (Knob 0) is always scanned, since it determines whether to enter the HOME state or the MENU state.

They are as follows:

- HOME

  - Scans the 12 keyboard keys
    - Keyboard keys are to play music
  - Scans Knob 2 and 3.
    - Knob 2 press toggles Master/Slave
    - Turning Knob 2 changes the octave
    - Knob 3 press toggles volume On/Off
    - Turning Knob 3 changes the volume

- MENU

  - Measures the joystick X values
    - Moving the joystick left/right shifts the menu items
  - Scans for joystick press
    - Pressing the joystick enters the chosen advanced feature

- DOOM

  - Measures the joystick X and Y values
    - Moving the joystick left/right and up/down controls character movement
  - Scans for joystick press
    - Pressing the joystick fires the gun

- WAVE SELECT

  - Measures the joystick X and Y values
    - Moving the joystick left/right and up/down controls which wave is selected
  - Scans for joystick press
    - Returns HOME, selecting this wave

- RECORD

  - Scans the 12 keyboard keys
    - Keyboard keys are to play music
  - Scans Knob 2 and 3.
    - Knob 2 press toggles Master/Slave
    - Turning Knob 2 changes the octave
    - Knob 3 press toggles volume On/Off
    - Turning Knob 3 changes the volume
  - Scans the joystick
    - Selects the track, and record or playback options

## Display Update Thread

To goal of this task is to display the user interface, so that the user can conveniently interact with the functionality.

Similarly to the Scan Keys task, the system wide state machine is accessed so that only the relevant data is displayed.

For more information on what is displayed, please consult the [Manual of Operation](/README.md).

## CAN Transmit Thread

The transmit task simply takes the `CAN_TX` semaphore, and sends a message, containing the press/release, key, octave, and volume across the CAN bus.

## CAN Decode Thread

The decode thread is triggered by the presence of input data in it's queue. If the input is:

- **Press**:

  - The phase accumulator for that voice is set to 0.
  - The current note pair (octave and key) is added to the notes vector (containing active notes).

- **Release**:

  - The notes vector is serached for the current octave and key pair
  - If the key pair is found, it is removed from the list of active notes.

There's also additional logic to control the volume and copy the last message to the `sysState` to be used elsewhere.

## Recording task

The recording task is designed to efficiently record a variable number of notes over a pre-defined time (5s as of writitng this). It can record up to 4 independent tracks, and play each of them back concurrently. The tracks can also be recorded whilst others play, to allow for you to time the tracks in line with eachother.

The recording works via a `playbackBuffer` which essentially forms a 3D array, containing a number of messages (which are arrays of size 8), over a number of tracks. It also contains an `activeNotes` 3D array, which again for each track, holds the current notes that are being pressed, by storing their octave and key. This is improtant later, as it is used to prevent keys getting stuck and being played at the edge cases of recording (e.g. starting recording midway through playback, stopping playback between press and releases).

The use of dynamic memory for storing the vectors described above isn't ideal in embedded systems, due to the likelihood of memory fragmentation occuring during prolonged operation, as well as the additional overhead of copying and reallocating the vector upon each addition or removal of elements. To reduce this, and improve the time complexity of operations we perform on these variables, we preallocate a suitable amount of memory for these vectors. This allows the vectors to work faster whilst occupying these areas, but also have the capacity to exceed them if necessary.

```c
for (int i = 0; i < 4 ; i++){
    playbackBuffer[i].reserve(100); //Reserves 100 spcaes in memory for each track (0.4kB overall)
    activeNotes.reserve(20); //Realistically not going to exceed 20 keys 
}
```

The playback buffer is reserved 100 elements which corresponds to 50 keys pressed in 5 seconds, which is a reasonable over estimation of the amount of notes someone could play in 5s. The active notes is reserved up to 20 notes to be active at any time (realistically would never exceed this, but it has the ability to have more than this). This reduces the time complexity, as reallocation to memory is unecessary. Vectors in C++ are assigned to contiguous memory, therefore in typical scenarios, this will also help prevent memory fragmentation.

After this, the recording simply copies RX messages that are stored in the `sysState` to the buffer, with a timestamp for the recorders internal counter assigned to the extra bits of the message. It ensures that the bits it uses to store the counter are empty before adding new values to the playback, to prevent it from recording keys that are being played by other recording tracks.

The playback then checks if each track is active, then itterates through each consecutive message in the track, using a pointer. If the timestamps match, the key is played.

During both recording and playback, whenever a key is pressed, it's octave and key are added to the active notes, and when they are released, they're removed from the active notes. When recording starts or ends, or when playback is stopped, any remaining keys that have not been released, are automatically released using the active keys. This helps prevent cases where the recording can get stuck on certain keys, if the playback or recording is changed between it's press and release.

If recording ends with keys that are unreleased, their releases are added to the end of hte recording too.

## Custom Wave Generation Task

We are using DMA to output a waveform of our choice through the onboard DAC. In order to have a continuously updating waveform, including one or more key presses, we use the a singular buffer which we access with DMA configured in circular mode. Compared to the double buffer method, for which the STM32L432KU6 doesn't have native support for, this method makes the reading and writing parts of the task to be very constricted in terms of timing.
Having one circular buffer instead of implementing a manual switching of the buffers means we avoid "glitches" in some parts of the waveform, as we can avoid the dead-time of having to stop and immediately restart the DAC in DMA mode.
Hence, on the same buffer, we will have one read head and one write head.
Using the ```HAL_DAC_ConvCpltCallback()``` and ```HAL_DAC_HalfConvCpltCallback()``` built-in interrupts, we can detect in which half of the buffer the read head of the DAC is.
These interrupts then set a flag which sets the position of the write head.
Additonally, the register callbacks release a semaphore, allowing the ```signalGenTask``` thread to write the non-read part of the buffer: this is a timing-critical part of the system. As such, we want it to be as low-latency and have a very low execution time.
<!-- Idk if to include initial attempts of dynamic buffer writing -->
In order to achieve this, we have a pre-computed period of the wave of the lowest frequency we will output in a buffer. This buffer is accessed via different phase accumulators, which result in different frequency waveforms added at the same time.
For each note pressed, for each element of the write half of the buffer, there is an addition, and a memory cell access, both of which are $O(1)$ time, resulting in an efficient $O(n)$ writing sequence. This is much faster than dynamically calculating each element. <!-- see about calling O(n) efficient lmao -->

Based on the sample timer that we use, $f_s$, we must choose an appropriate buffer size for real-time performance.

Setting the hardware timer to a sampling frequency $f_s$ with a DAC buffer of size $N$ means that the `signalGenTask` thread must fill its designated half of the buffer and be ready for the next one in at most $\frac{N}{2}\frac{1}{f_s}$ seconds.
Hence, assuming this task (which is of a relatively high priority) does not get pre-empted, the minimum initiation interval is:

$$\tau = \frac{N}{2}\frac{1}{f_s}$$

We can predict the maximum possible execution time. Let the number of keys pressed $k$, and the time it takes to calculate the next index on the LUT from the phase accumulator and add it to the buffer be $\delta t$. The total time it would take to fill half the buffer for $k$ keys pressed is, and hence the maximum execution time is:

$$T = k\times\frac{N}{2}\times\delta t$$

<!-- below may be useless -->
Additionally, it is possible to determine an optimal LUT size, leveraging storage space and performance. Taking a natural A note to be 440Hz, a C is then a 242Hz tone, in an equal temperament keyboard. This means the lowest note we can achieve is 3 octaves lower at $\frac{242}{2^3} = 30Hz$. The normalised angular period, ie. the minimum LUT size, is therefore:

$$N' = \frac{f_s}{2\pi\times 30} = 116$$

In practice however, we have chosen to increase that a power of 2, in order to make the bit shifting in the phase accumulator more straightforward.
Note that the lowest frequency is barely above the limit of human hearing capabilities, and we do not anticipate using at octave 1, at least for regular sine waves, but usage of the keyboard at this frequency with a non-optimal LUT size may result in aliasing, making the output wave be perceived as a higher frequency wave.
