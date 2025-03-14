<!-- Sorry about the horrendous naming -->
# Minimum Initiation Intervals & Maximum Execution Time Analysis

## 1. 



## X. Custom Wave Generation Task
Setting the hardware timer to a sampling frequency $f_s$ with a DAC buffer of size $N$ means that the `signalGenTask` thread must fill its designated half of the buffer and be ready for the next one in at most $\frac{N}{2}\frac{1}{f_s}$ seconds.
Hence, assuming this task (which is of a relatively high priority) does not get pre-empted, the minimum initiation interval is:

$
\tau = $\frac{N}{2}\frac{1}{f_s}$
$

We can predict the maximum possible execution time. Let the number of keys pressed $k$, and the time it takes to calculate the next index on the LUT from the phase accumulator and add it to the buffer be $\delta t$. The total time it would take to fill half the buffer for $k$ keys pressed is, and hence the maximum execution time is:

$
T = k\times\frac{N}{2}\times\delta t
$

<!-- below may be useless -->
Additionally, it is possible to determine an optimal LUT size, leveraging storage space and performance. Taking a natural A note to be 440Hz, a C is then a 242Hz tone, in an equal temperament keyboard. This means the lowest note we can achieve is 4 octaves lower at $\frac{242}{2^4} = 15Hz$. The normalised angular period, ie. the maximum LUT size, is therefore:

$
N' = \frac{f_s}{2\pi\times 15}
$

Note that this is below the limit of human hearing capabilities, and we do not anticipate using at octave 0, but usage of the keyboard at this frequency with a non-optimal LUT size may result in aliasing, making the output wave be perceived as a higher frequency wave.