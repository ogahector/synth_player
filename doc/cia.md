# Critical Instant Analysis of the Rate Monotonic Scheduler

Critial Instant Analysis (CIA) evaluates the worst-case response time of the lowest-priority task (Display Update) when all tasks are released simultaneously at $ t = 0 $ (the critical instant). The total latency must not exceed the 100ms deadline.

## Formula

For each task $i$, the number of executions within 100ms is:
$$N_i = \lceil \frac{T}{\tau_i} \rceil$$
where $N_i$ is the number of times task i occurs in the highest initation interval, $T = 100\text{ms}$ (analysis window), and $\tau_i$ is the task’s initition interval. The overall latency is:
$$\text{Latency} = \sum_i (L_i \times T_i)$$
where $T_i$ is the WCET for task i.

## Step-by-Step Calculation

WCETs were taken from [Initiation Interval & Maximum Execution Time Analysis](doc/ii_met.md).

- **Scan Keys**:  $\lceil\frac{100}{10}\rceil = 10$ , $\implies N = 10 \times 1.44 = 14.40 \, \text{ms}$
- **Record**: $\lceil\frac{100}{10}\rceil = 10$ , $\implies N = 10 \times 0.00287 = 0.0287 \text{ms}$
- **Sig Gen**: $\lceil\frac{100}{17}\rceil = 6$ , $\implies N = 6 \times 2.06 = 12.36  \text{ms}$
- **Transmit**: $\lceil\frac{100}{25.2}\rceil = 4$ , $\implies N = 4 \times 0.7 = 2.8  \text{ms}$
- **Decode**: $\lceil\frac{100}{30}\rceil = 4$ , $\implies N = 4 \times 0.00322 = 0.0128 \text{ms}$
- **Display Update**: $\lceil\frac{100}{100}\rceil = 1$ , $\implies N = 1 \times 17.6 = 17.6  \text{ms}$

**Total Latency**:
$14.40 + 0.0287 + 12.36 + 2.8 + 0.0128 + 17.6 = 47.2015 \, \text{ms} \approx 47.3 \, \text{ms}$

## Analysis

- **Result**: 47.3ms < 100ms, leaving a 52.7 ms margin for overheads (e.g., context switching, interrupt handling).
- **Overhead Margin**: FreeRTOS context switching typically takes microseconds (e.g., 10-50µs per switch). With approximately 35 task switches (sum of $ N_i $), assuming 50µs each, overhead is ~1.75ms, well within the margin.
- **Worst-Case Scenario**: This assumes all tasks execute at their WCET simultaneously, an unlikely event due to event-driven tasks (e.g., Decode, Transmit) not always triggering at maximum frequency.
- **Schedule Jitter**: We assume here that the schedule jitter only occurs on the highest priority scheduled task, adding 1 ms to the scan keys task. However, if we were to assume that the schedule jitter were to occur on all scheduled tasks, this would increase our CIA value by 11ms (10ms for the recording task, and 1ms for the dsplay update task, all others are not scheduled by FreeRTOS, and are event driven).

<!-- Should add CIA gantt chart here -->

## Assumptions

The analysis relies on several assumptions, expanded here for clarity:

- **No Switching Overheads**: RMS assumes zero context-switching time. In FreeRTOS, overhead is minimal (e.g., 50µs per switch), validated by the 15.4ms margin.
- **No Dependencies**: Assumes tasks are independent. In reality:
  - **Scan Keys → Decode**: Scan Keys enqueues events for Decode via a queue.
  - **Decode → Sig Gen**: Decoded notes trigger waveform generation.
  - Managed via FreeRTOS queues and semaphores, avoiding priority inversion.
- **Fixed Intervals**: Assumes fixed periods, but:
  - **Decode/Transmit**: Event-driven; intervals (30ms, 25.2ms) are worst-case estimates based on Scan Keys (10ms) driving 12 key events.
  - **Sig Gen**: Triggered by DAC interrupts; 17ms derived from buffer size and sample rate.
- **Omitted States**:
  - **DOOM State**: Excluded as non-critical; its rendering engine operates independently.
  - **MENU State**: Exceeds 100ms intentionally for smooth transitions, acceptable as it’s not a real-time constraint.
- **Worst-Case Scenarios**:
  - **Scan Keys**: All keys pressed simultaneously.
  - **Decode**: Maximum queue load (36 events).
  - **Record**: Full recording/playback of 400 values (100 per track).
  - **Sig Gen**: 20 voices active.
  - **Transmit**: Continuous CAN transmissions.
  - **Display Update**: Dynamic elements at peak load.

<!-- Should also add a section on uncertainty in our system, can comment on how vectors are typically uncertain so we prallocate memory to them to reduce time complexity to O(n). (Probably goes in the data bit). -->
