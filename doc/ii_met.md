<!-- Sorry about the horrendous naming -->
# Minimum Initiation Intervals & Maximum Execution Time Analysis

## Tasks and their characteristics

The system comprises six periodic or event-driven tasks implemented as threads or interrupts. Below is an expanded description of each task, including their type, initiation intervals (periods), worst-case execution times (WCET), and additional details to enhance clarity.


### 1. Scan Keys Task

- Type: Thread
- Initiation Interval / Deadline: 10ms
- Execution Time (WCET): 0.44ms (440µs) + 1ms for schedule jitter (since this is the highest priority scheduled thread)
- Description: Periodically scans the keyboard to detect key presses and releases. Implemented as a thread for regular polling.
- Measurement Method: Execution time measured using a high-resolution timer over multiple calls to the task, capturing the worst case when all keys are pressed simultaneously on the keyboard.
- Rationale: A 3ms interval ensures no perceptible delay (specification 2), as human perception typically notices delays above 10-20ms.

### 2. Record Task

- Type: Thread
- Initiation Interval / Deadline: 10ms
- Execution Time (WCET): 0.00287ms (2.87µs)
- Description: Records key presses for playback, supporting up to four tracks with 100 preallocated values each (can exceed 100 if necessary).
- Measurement Method: WCET obtained by profiling simultaneous recording and playback of all tracks with maximum data.
- Rationale: A 10ms interval balances responsiveness and memory efficiency for recording functionality.

### 3. Signal Generator Task
- Type: Thread
- Initiation Interval / Deadline: 17ms , derived from $\frac{N}{2}\times\frac{1}{f_s}$ where $N = 750$ is the size of our buffer, and $f_s = 22kHz$ is the frequency of the DAC buffer.
- Execution Time (WCET): 2.06ms
- Description: Generates sine, squaure, sawtooth or triangle waveforms for up to 108 voices, triggered by DAC interrupts when the buffer is half or fully consumed.
- Measurement Method: WCET measured with at most 20 voices active, simulating maximum polyphony. Though we assume the average keyboard will only see one user at a time, limiting the maximum number of keys pressed to 10, we account for the edge case that the users may play a piano duet simultaneously, though unlikely.
- Rationale: The 17ms period reflects the DAC buffer refresh rate, ensuring continuous audio output.

### 4. Transmit Task

- Type: Interrupt
- Initiation Interval / Deadline: 25.2ms for 36 executions (average 0.7ms per execution)
- Execution Time (WCET): 0.7ms
- Description: Sends CAN bus messages when configured as a sender, triggered by key events.
- Measurement Method: WCET is derrived Lab 2; CAN frame transmission $\simeq 0.7ms$.
- Rationale: The 25.2ms interval assumes 36 events over a period, derived similarly to Decode, adjusted for CAN bus timing.

### 5. Decode Task

- Type: Thread (triggered from queue by interrupt)
- Initiation Interval / Deadline: 30ms for 36 executions (average 0.25ms per execution)
- Execution Time (WCET): 0.00322ms (3.22µs)
- Description: Decodes key press/release events from a queue populated by an interrupt, determining the corresponding note and octave.
- Measurement Method: Measured via profiling during maximum queue activity (e.g., all 12 keys pressed in rapid succession).
- Rationale: The 30ms interval for 36 executions assumes the queue is driven by Scan Keys (10ms period). If 12 keys are pressed over 10ms, each key event takes approximately 0.833ms, totaling 30ms for a full cycle.

### 6. Display Update Task

- Type: Thread
- Initiation Interval / Deadline: 100ms
- Execution Time (WCET): 17.6ms
- Description: Refreshes the OLED display with note names, volume levels and other menu elements.
- Measurement Method: WCET measured with dynamic elements (e.g., moving icons) at maximum update rate.
- Rationale: The 100ms deadline aligns with the specification, ensuring timely visual feedback.

Note: The Precompute Sig Gen Values task (121ms at startup) is excluded from real-time analysis as it runs once during initialization, not as a periodic task.

## Priority

Using Rate Monotonic Scheduling (RMS), tasks with shorter periods receive higher priorities to optimize CPU utilization and ensure schedulability. The priority order is:

1. Scan Keys (10ms)
2. Record (10ms)
3. Sig Gen (17ms)
4. Transmit (25.2ms)
5. Decode (30ms)
6. Display Update (100ms)

RMS is ideal for fixed-priority, single-core systems like the STM32L432KU6, maximizing CPU utilization by prioritizing tasks with higher execution frequencies. Dependencies (e.g., Decode relying on Scan Keys) are managed via queues, not priority inversion, as discussed later.
