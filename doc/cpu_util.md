# CPU Utilization

CPU utilization quantifies the system’s workload under RMS.

## Formula

$U = \sum \frac{T_i}{\tau_i} $

## Calculation

- **Scan Keys**: $\frac{1.44}{10}\approx 0.144$ (14.4%)
- **Record**: $\frac{0.00287}{10}\approx 0.000287$ (0.0287%)
- **Sig Gen**: $\frac{2.06}{17}\approx 0.1212$ (12.124%)
- **Transmit**: $\frac{0.7}{25.2}\approx 0.0278$ (2.78%)
- **Decode**: $\frac{0.00322}{30}\approx 0.000107$ (0.0107%)
- **Display Update**: $\frac{17.6}{100}\approx 0.176$ (17.6%)

**Total**: $0.144 + 0.000287 + 0.1212 + 0.0278 + 0.000107 + 0.176\approx 0.4696$ (46.96%)

## Analysis

- **Result**: 46.93% utilization indicates a well balanced and schedulable system. 
- **RMS Bound**: For 6 tasks, the theoretical bound is $n(2^{1/n} - 1) = 6(2^{1/6} - 1)\approx 0.723$ (72.3%). We do not exceed this (46.96%), which helps ensure our schedulability; CIA confirms deadlines are met.
- **Practical Note**: Actual utilization may be lower if tasks don’t always execute at WCET or maximum frequency.
