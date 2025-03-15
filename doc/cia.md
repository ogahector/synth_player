# Critical Instant Analysis of the Rate Monotonic Scheduler

Let us list the tasks, in no particular order, and their initiation interval. For rate-monotonic scheduling, we will assume a fixed initiation interval and deadline, no dependencies or switching overheads. The STM32L432KU6 board indeed only has one CPU core, and RTOS task scheduling setups also only allow for fixed task priorities. Hence, if we allocated the shortest initiation interveral to the highest priority, we will have optimal CPU utilisation.
