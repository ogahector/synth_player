# Task Identification & Description

All the tasks implemented on this board are:
1. [Scan Keys Thread](#scan-keys-thread)
2. [Display Update Thread](#display-update-thread)
3. [CAN Decode Thread](#can-decode-thread)
4. [CAN Encode Thread](#can-encode-thread)
5. [Wave Generation Task](#custom-wave-generation-task)


## Scan Keys Thread

## Display Update Thread

## CAN Encode Thread

## CAN Decode Thread

## Custom Wave Generation Task

We are using DMA to output a waveform of our choice through the onboard DAC. In order to have a continuously updating waveform, including one or more key presses, we use the a singular buffer which we access with DMA configured in circular mode. Compared to the double buffer method, for which the STM32L432KU6 doesn't have native support for, this method makes the reading and writing parts of the task to be very constricted in terms of timing. 
Having one circular buffer instead of implementing a manual switching of the buffers means we avoid "glitches" in some parts of the waveform, as we can avoid the dead-time of having to stop and immediately restart the DAC in DMA mode. 
Hence, on the same buffer, we will have one read head and one write head.
Using the ```HAL_DAC_ConvCpltCallback()``` and ``` HAL_DAC_HalfConvCpltCallback()``` built-in interrupts, we can detect in which half of the buffer the read head of the DAC is. 
These interrupts then set a flag which sets the position of the write head.
Additonally, the register callbacks release a semaphore, allowing the ```signalGenTask``` thread to write the non-read part of the buffer: this is a timing-critical part of the system. As such, we want it to be as low-latency and have a very low execution time.
<!-- Idk if to include initial attempts of dynamic buffer writing -->
In order to achieve this, we have a pre-computed period of the wave of the lowest frequency we will output in a buffer. This buffer is accessed via different phase accumulators, which result in different frequency waveforms added at the same time. 
For each note pressed, for each element of the write half of the buffer, there is an addition, and a memory cell access, both of which are $O(1)$ time, resulting in an efficient $O(n)$ writing sequence. This is much faster than dynamically calculating each element. <!-- see about calling O(n) efficient lmao -->

Based on the sample timer that we use, $f_s$, we must choose an appropriate buffer size for real-time performance. 
