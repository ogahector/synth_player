# ES-synth-starter

  Use this project as the starting point for your Embedded Systems labs and coursework.
  
  [Lab Part 1](doc/LabPart1.md)
  
  [Lab Part 2](doc/LabPart2.md)

## Additional Information

  [Handshaking and auto-detection](doc/handshaking.md)
  
  [Double buffering of audio samples](doc/doubleBuffer.md)

  [StackSynth V1.1 Schematic](doc/StackSynth-v1.pdf)

  [StackSynth V2.1 Schematic](doc/StackSynth-v2.pdf)

## Possible Improvements For the Demo
1. Optimise per-thread stack size
  - dynamically allocated memory, refactoring, etc
  - use both recommended methods

2. Use inline / static functions vs non-static

3. Timing Optimisation
  - make tighter timings?

4. General Code Cleanliness
  - i think he'll acc look at it this time
  - #define instead of const int for pin defs 

5. Finite Semaphore Ticks
  - copy contents of sysState into local, and only access it then?
    - see how that compares without copying