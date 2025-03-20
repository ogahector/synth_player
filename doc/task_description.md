# Task Identification & Description

All the tasks implemented on this board are:

1. [Scan Keys Thread](#scan-keys-thread)
2. [Display Update Thread](#display-update-thread)
3. [CAN Decode Thread](#can-decode-thread)
4. [CAN Encode Thread](#can-encode-thread)
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

- HOME
    - Displays the volume level with a level bar
    - Displays the octave
    - Displays whether the current board is a master or slave
    - Shows what key is pressed when pressed
    - Displays the CAN bus message received or sent

- [MENU](menu.md)
    - Displays the DOOM icon initially, which can be clicked (using the joystick) to enter Doom
    - When the joystick is moved left or right, the next icon slides in to take the place of the old one
    - Allows to select between Doom, Wave Select and Recording

- [DOOM](doom.md)
    - Renders our own version of the 1993 popular video game Doom
    - Displays enemies, obstacles (trees), and bullet animation, in a 2.5D rendered world (forward/backward and left/right movement)
    - Displays the frames per second (FPS) of the gameplay
- WAVE SELECT
    - Displays a grid of 4 different waveforms
        - Sine wave, square wave, sawtooth wave, triangle wave
- RECORD
     - First screen displays a grid of 4 tracks to select
     - Second screen shows the selected track, button to record or play, and a back button.
        - When record or play are pressed, appropriate icons show what was selected

## CAN Encode Thread

## CAN Decode Thread

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
