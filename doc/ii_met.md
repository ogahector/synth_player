<!-- Sorry about the horrendous naming -->
# Minimum Initiation Intervals & Maximum Execution Time Analysis

## Tasks and their characteristics

The system comprises six periodic or event-driven tasks implemented as threads or interrupts. Below is an expanded description of each task, including their type, initiation intervals (periods), worst-case execution times (WCET), and additional details to enhance clarity.

### 1. Scan Keys Task

- Type: Thread
- Initiation Interval / Deadline: 3ms
- Execution Time (WCET): 0.44ms (440µs)
- Description: Periodically scans the keyboard to detect key presses and releases. Implemented as a thread for regular polling.
- Measurement Method: Execution time measured using a high-resolution timer over multiple calls to the task, capturing the worst case when all keys are pressed simultaneously.
- Rationale: A 3ms interval ensures no perceptible delay (specification 2), as human perception typically notices delays above 10-20ms.

### 2. Decode Task

- Type: Thread (triggered from queue by interrupt)
- Initiation Interval / Deadline: 9ms for 36 executions (average 0.25ms per execution)
- Execution Time (WCET): 0.0008ms (0.8µs)
- Description: Decodes key press/release events from a queue populated by an interrupt, determining the corresponding note and octave.
- Measurement Method: Measured via profiling during maximum queue activity (e.g., all 12 keys pressed in rapid succession).
- Rationale: The 9ms interval for 36 executions assumes the queue is driven by Scan Keys (3ms period). If 12 keys are pressed over 3ms, each key event takes approximately 0.25ms, totaling 9ms for a full cycle.

### 3. Record Task

- Type: Thread
- Initiation Interval / Deadline: 10ms
- Execution Time (WCET): 0.0022ms (2.2µs)
- Description: Records key presses for playback, supporting up to four tracks with 100 preallocated values each (can exceed 100 if necessary).
- Measurement Method: WCET obtained by profiling simultaneous recording and playback of all tracks with maximum data.
- Rationale: A 10ms interval balances responsiveness and memory efficiency for recording functionality.

### 4. Signal Generator Task

Setting the hardware timer to a sampling frequency $f_s$ with a DAC buffer of size $N$ means that the `signalGenTask` thread must fill its designated half of the buffer and be ready for the next one in at most $\frac{N}{2}\frac{1}{f_s}$ seconds.
Hence, assuming this task (which is of a relatively high priority) does not get pre-empted, the minimum initiation interval is:

$\tau = \frac{N}{2}\frac{1}{f_s}$

We can predict the maximum possible execution time. Let the number of keys pressed $k$, and the time it takes to calculate the next index on the LUT from the phase accumulator and add it to the buffer be $\delta t$. The total time it would take to fill half the buffer for $k$ keys pressed is, and hence the maximum execution time is:

$T = k\times\frac{N}{2}\times\delta t$

<!-- below may be useless -->
Additionally, it is possible to determine an optimal LUT size, leveraging storage space and performance. Taking a natural A note to be 440Hz, a C is then a 242Hz tone, in an equal temperament keyboard. This means the lowest note we can achieve is 4 octaves lower at $\frac{242}{2^4} = 15Hz$. The normalised angular period, ie. the maximum LUT size, is therefore:

$N' = \frac{f_s}{2\pi\times 15}$

Note that this is below the limit of human hearing capabilities, and we do not anticipate using at octave 0, but usage of the keyboard at this frequency with a non-optimal LUT size may result in aliasing, making the output wave be perceived as a higher frequency wave.

- Type: Thread
- Initiation Interval / Deadline: 17ms , derived from $\frac{N}{2}\times\frac{1}{f_s}$ where $N = 750$ is the size of our buffer, and $f_s = 22kHz$ is the frequency of the DAC buffer.
- Execution Time (WCET): 2.06ms
- Description: Generates sine, squaure, sawtooth or triangle waveforms for up to 108 voices, triggered by DAC interrupts when the buffer is half or fully consumed.
- Measurement Method: WCET measured with all 108 voices active, simulating maximum polyphony.
- Rationale: The 17ms period reflects the DAC buffer refresh rate, ensuring continuous audio output.

### 5. Transmit Task

- Type: Interrupt
- Initiation Interval / Deadline: 25.2ms for 36 executions (average 0.7ms per execution)
- Execution Time (WCET): 0.7ms
- Description: Sends CAN bus messages when configured as a sender, triggered by key events.
- Measurement Method: WCET is derrived Lab 2; CAN frame transmission $\simeq 0.7ms$.
- Rationale: The 25.2ms interval assumes 36 events over a period, derived similarly to Decode, adjusted for CAN bus timing.

### 6. Display Update Task

- Type: Thread
- Initiation Interval / Deadline: 100ms
- Execution Time (WCET): 17.6ms
- Description: Refreshes the OLED display with note names, volume levels and other menu elements.
- Measurement Method: WCET measured with dynamic elements (e.g., moving icons) at maximum update rate.
- Rationale: The 100ms deadline aligns with the specification, ensuring timely visual feedback.

Note: The Precompute Sig Gen Values task (121ms at startup) is excluded from real-time analysis as it runs once during initialization, not as a periodic task.

## Priority

Using RMS, tasks with shorter periods receive higher priorities to optimize CPU utilization and ensure schedulability. The priority order is:

1. Scan Keys (3ms)
2. Decode (9ms)
3. Record (10ms)
4. Sig Gen (17ms)
5. Transmit (25.2ms)
6. Display Update (100ms)

Justification: RMS is ideal for fixed-priority, single-core systems like the STM32L432KU6, maximizing CPU utilization by prioritizing tasks with higher execution frequencies. Dependencies (e.g., Decode relying on Scan Keys) are managed via queues, not priority inversion, as discussed later.
