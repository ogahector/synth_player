# Critical Instant Analysis of the Rate Monotonic Scheduler

Let us list the tasks, in no particular order, and their initiation interval. For rate-monotonic scheduling, we will assume a fixed initiation interval and deadline, no dependencies or switching overheads. The STM32L432KU6 board indeed only has one CPU core, and RTOS task scheduling setups also only allow for fixed task priorities. Hence, if we allocated the shortest initiation interveral to the highest priority, we will have optimal CPU utilisation.

## Tasks

Our tasks are defined as below, with their initiation intervals, and their calculated exection times. 
#### Task : Scan keys
Type : Thread
Initiation Interval / Deadline - 3ms
Execution time (worst case) - 0.44ms

<!-- Need to ensure the worst case on this one -->
#### Task : Decode
Type : Thread (Triggered from queue by interrupt)
Initiation Interval / Deadline - 9ms for 36 executions (Lab 2 notes for reference)
Execution time - 0.0008 ms (0.8 us)

#### Task : Record
Type : Thread
Initiation Interval / Deadline - 10ms
Execution time - 0.0022 ms (2.2 us)

#### Task : Sig Gen
Type : Thread
Initiation Interval / Deadline - 17ms (from N/2 * 1/fs)
Execution time (worst case, all 108 voices at once) - 8.2ms

#### Task : Transmit
Type : Interrupt
Initiation Interval / Deadline - 25.2ms for 36 executions (Lab 2 notes for reference)
Execution time - 0.7ms (from lab 2, in practice, all blocking function)

#### Task : Display update
Type : Thread
Initiation Interval / Deadline - 100ms
Execution time (worst case) - 17.6ms

<!-- Likely want to remove this  -->
#### Task : Precompute Sig Gen values
Type : N/A
Initiation Interval / Deadline - N/A
Execution time - 121 ms (not a thread or interrupt, calculated on startup)

<!-- Testing all tasks at once, to see the longest possible time taken (assuming worst case for each) gives : 26.3ms overall, which meets the 100ms requirement.  -->

CAN frames take <1ms to transmit

## Priority List 
Assigning the highest priority to tasks with the lowest initation intervals gives the following :
1. Scan Keys
2. Decode 
3. Record
4. Sig Gen
5. Transmit 
6. Display Update

## Critical Instant Analysis

Doing critical instant analysis, with these assigned priorites, gives the following result:
$L_1 = ceiling(\frac{100}{3}) = 34$
$L_2 = ceiling (\frac{100}{9}) = 12$ 
$L_3 = ceiling (\frac{100}{10}) = 10$ 
$L_4 = ceiling (\frac{100}{17}) = 6$
$L_5 = ceiling (\frac{100}{25.2}) = 4$
$L_6 = ceiling (\frac{100}{50}) = 1$
$Overall Latency = 34 \times T_1 + 12 \times T_2 + 10 \times T_3 + 6 \times T_4 + 4 \times T_5 + T_6 = 87ms $

82ms <100ms so the system will never miss a deadline, however there isn't a lot of wiggle room, only around 13ms. This is still <90% of the deadline, so there should still be enough room for scheduler overheads, additionally, this worst case scenario is very unlikely to happen, due to the assumptions made on each test (see assumptions section below for further details). 

## CPU Utilisation
With the system described above, the CPU utilisation can be calculated via the formula :
$Util = \sum{\frac{T_i}{\tau_i}}$ where $T_i$ is the execution time, and $\tau_i$ is the initiation interval of i. Using this, we can determine the CPU utilisation to be 84%. 

<!-- Should add CIA diagram here -->

## Assumptions 
To reach the conclusions made above, several assumptions had to be made on the behaviour of the system. 
- Firstly, the assumption made with Rate Monotonic Scheduling is that there are no swithcing overheads, when in reality, there will be some. Granted, the impact of these is limited with FreeRTOS, however thier ommition is still a key assumption. 
- The second assumption made with RMS is the lack of dependencies. Several of these tasks are linked together, and require events or information to be released from another before they can correctly complete their own task. Dependencies require their own analysis entirely, and are expanded upon in the deadlock section.
- The assumption that inititation intervals are fixed is only true for a selection of these tasks, some of them are event driven:
    - The decoder and transmitter are driven by the presence of data in their respective queues, and therefore have no set initiation interval. The initiation intervals given above for these, are derrived from the fact that the queue's are driven by primarily the scan keys task, which occurs every 3ms, and if you were to press all 12 keys during this time, you'd get 1 key = 0.25ms, and if you were to use this assumption to fill the queue, it would take 9ms (similar method for the transmit task).
    - The Sig gen task is another task that doesn't have a fixed schedule. It's 'triggered' by the DAC interrupts, which release a signal mutex when the DAC has read half or the full size of the buffer, which indicate the next write should occur. As a result, the initiation interval of this can be determined from the equation $\tau = \frac{N}{2}\frac{1}{f_s}$
- Certain states of these tasks were omitted from the analysis for a variety of reasons
    - The DOOM state of the update display task was omitted, due to the fact it is not critical to the systems use or performance, and has it's own specific frames and engine to render and run images, which are not necessarily designed to work alongside other tasks.
    - The MENU state of the display update was also omitted, as it deliberately exceeds the 100ms deadline, in order to achieve a smooth transition between icons shown on the screen
    <!-- Check that the yap above is true, and if it is, expand upon why  we can exceed the 100ms deadline with no impact? -->
- The 'worst case' scenarios for each task were determined as below:
    - For the scan keys task, we assume all keys are pressed simulatenously
    - Deocde <---- DO THIS
    - For the recording task, we test the system while recording every call, playing back all 4 tracks every call, with 100 values in each track, and we also test while recording and playing back to find the worst case
    - For the signal generator, we assume every possible key, from every possible octave is pressed at once (108 keys)
    - Transmit <--- ?
    - For the display task, we simply run each different display menu, and if it has dynamic elements (e.g. moving icons, highlighting selections), we run these elements as fast as possible repeatedly.
    

Should also add a section on uncertainty in our system, can comment on how vectors are typically uncertain so we prallocate memory to them to reduce time complexity to O(n). (Probably goes in the data bit).
