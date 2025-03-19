# CPU Utilization

CPU utilization quantifies the system’s workload under RMS.

## Formula
$U = \sum \frac{T_i}{\tau_i} $

## Calculation
- **Scan Keys**: $\frac{0.44}{3}\approx 0.1467$ (14.67%)
- **Decode**: $\frac{0.0008}{9}\approx 0.000089$ (0.0089%)
- **Record**: $\frac{0.0022}{10}\approx 0.00022$ (0.022%)
- **Sig Gen**: $\frac{8.2}{17}\approx 0.4824$ (48.24%)
- **Transmit**: $\frac{0.7}{25.2}\approx 0.0278$ (2.78%)
- **Display Update**: $\frac{17.6}{100}\approx 0.176$ (17.6%)

**Total**: $0.1467 + 0.000089 + 0.00022 + 0.4824 + 0.0278 + 0.176\approx 0.8332$ (83.32%)

## Analysis
- **Result**: 83.32% utilization indicates a heavily loaded but schedulable system.
- **RMS Bound**: For 6 tasks, the theoretical bound is $n(2^{1/n} - 1) = 6(2^{1/6} - 1)\approx 0.723$ (72.3%). Exceeding this (83.32%) doesn’t guarantee unschedulability; CIA confirms deadlines are met.
- **Practical Note**: Actual utilization may be lower if tasks don’t always execute at WCET or maximum frequency.