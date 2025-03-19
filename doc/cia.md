# Critical Instant Analysis of the Rate Monotonic Scheduler

Critial Instant Analysis (CIA) evaluates the worst-case response time of the lowest-priority task (Display Update) when all tasks are released simultaneously at $ t = 0 $ (the critical instant). The total latency must not exceed the 100ms deadline.

## Formula

For each task $i$, the number of executions within 100ms is:
$$L_i = \lceil \frac{T}{\tau_i} \rceil$$
where $T = 100\text{ms}$ (analysis window), and $\tau_i$ is the task’s initition interval. The overall latency is:
$$\text{Latency} = \sum_i (L_i \times T_i)$$
where $T_i$ is the WCET for task i.

## Step-by-Step Calculation

WCETs were taken from [Initiation Interval & Maximum Execution Time Analysis](doc/ii_met.md).

- **Scan Keys**:  $\lceil\frac{100}{3}\rceil = 34$ , $\implies L = 34 \times 0.44 = 14.96 \, \text{ms}$
- **Decode**: $\lceil\frac{100}{9}\rceil = 12$ , $\implies L = 12 \times 0.0008 = 0.0096 \text{ms}$
- **Record**: $\lceil\frac{100}{10}\rceil = 10$ , $\implies L = 10 \times 0.0022 = 0.022 \text{ms}$
- **Sig Gen**: $\lceil\frac{100}{17}\rceil = 6$ , $\implies L = 6 \times 2.06 = 12.36  \text{ms}$
- **Transmit**: $\lceil\frac{100}{25.2}\rceil = 4$ , $\implies L = 4 \times 0.7 = 2.8  \text{ms}$
- **Display Update**: $\lceil\frac{100}{100}\rceil = 1$ , $\implies L = 1 \times 17.6 = 17.6  \text{ms}$

**Total Latency**:
$14.96 + 0.0096 + 0.022 + 49.2 + 2.8 + 17.6 = 84.5916 \, \text{ms} \approx 84.6 \, \text{ms}$

## Analysis

- **Result**: 84.6ms < 100ms, leaving a 15.4ms margin for overheads (e.g., context switching, interrupt handling).
- **Overhead Margin**: FreeRTOS context switching typically takes microseconds (e.g., 10-50µs per switch). With approximately 66 task switches (sum of $ L_i $), assuming 50µs each, overhead is ~3.3ms, well within the margin.
- **Worst-Case Scenario**: This assumes all tasks execute at their WCET simultaneously, an unlikely event due to event-driven tasks (e.g., Decode, Transmit) not always triggering at maximum frequency.

<!-- Should add CIA gantt chart here -->

## Assumptions

The analysis relies on several assumptions, expanded here for clarity:

- **No Switching Overheads**: RMS assumes zero context-switching time. In FreeRTOS, overhead is minimal (e.g., 50µs per switch), validated by the 15.4ms margin.
- **No Dependencies**: Assumes tasks are independent. In reality:
  - **Scan Keys → Decode**: Scan Keys enqueues events for Decode via a queue.
  - **Decode → Sig Gen**: Decoded notes trigger waveform generation.
  - Managed via FreeRTOS queues and semaphores, avoiding priority inversion.
- **Fixed Intervals**: Assumes fixed periods, but:
  - **Decode/Transmit**: Event-driven; intervals (9ms, 25.2ms) are worst-case estimates based on Scan Keys (3ms) driving 12 key events.
  - **Sig Gen**: Triggered by DAC interrupts; 17ms derived from buffer size and sample rate.
- **Omitted States**:
  - **DOOM State**: Excluded as non-critical; its rendering engine operates independently.
  - **MENU State**: Exceeds 100ms intentionally for smooth transitions, acceptable as it’s not a real-time constraint.
- **Worst-Case Scenarios**:
  - **Scan Keys**: All keys pressed simultaneously.
  - **Decode**: Maximum queue load (36 events).
  - **Record**: Full recording/playback of 400 values.
  - **Sig Gen**: 108 voices active.
  - **Transmit**: Continuous CAN transmissions.
  - **Display Update**: Dynamic elements at peak load.

<!-- Should also add a section on uncertainty in our system, can comment on how vectors are typically uncertain so we prallocate memory to them to reduce time complexity to O(n). (Probably goes in the data bit). -->
