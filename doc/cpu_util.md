# CPU Utilization

CPU utilization quantifies the system’s workload under RMS.

## Formula
\[ U = \sum \frac{C_i}{P_i} \]

## Calculation
- **Scan Keys**: \( \frac{0.44}{3} \approx 0.1467 \) (14.67%)
- **Decode**: \( \frac{0.0008}{9} \approx 0.000089 \) (0.0089%)
- **Record**: \( \frac{0.0022}{10} = 0.00022 \) (0.022%)
- **Sig Gen**: \( \frac{8.2}{17} \approx 0.4824 \) (48.24%)
- **Transmit**: \( \frac{0.7}{25.2} \approx 0.0278 \) (2.78%)
- **Display Update**: \( \frac{17.6}{100} = 0.176 \) (17.6%)

**Total**: \( 0.1467 + 0.000089 + 0.00022 + 0.4824 + 0.0278 + 0.176 \approx 0.8332 \) (83.32%)

## Analysis
- **Result**: 83.32% utilization indicates a heavily loaded but schedulable system.
- **RMS Bound**: For 6 tasks, the theoretical bound is \( n(2^{1/n} - 1) = 6(2^{1/6} - 1) \approx 0.723 \) (72.3%). Exceeding this (83.32%) doesn’t guarantee unschedulability; CIA confirms deadlines are met.
- **Practical Note**: Actual utilization may be lower if tasks don’t always execute at WCET or maximum frequency.

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