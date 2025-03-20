# Deadlock and dependencies

## Dependencies

The dependencies we determined for our system are shown in the diagram below :
![Dependencies](/Images/dependencies.png)

As you can see above, we have a few blocking dependencies between tasks. Below we'll discuss each of them, and their impacts:

- **Scan Keys** - Scan keys has no dependencies, but it is the source of a few different dependencies
- **Transmit** - The transmit task relies on inputs from either the scan keys task (if the board is set up a slave), or the recording task. This dependency is managed via a FreeRTOS queue, enabling asynchronous communication. The queue ensures that Transmit can wait for data without holding resources, reducing the risk of blocking-related issues.
- **Decode** - The decode task will also depend on inputs from it's queue to function. These inputs can come from both the transmit task (via the CAN bus), or directly from the scan keys task (if the board is configured as a master). Similarly to the transmit, the use of a queue helps prevent directly blocking other tasks.
- **Update Display** - The update display task relies on a few different tasks to control it's output. It relies on the Scan keys task to determine if it is within the menu state, or the home state. And it also relies on the decode task to update it's message output (when in the home state). These dependencies are managed through shared variables protected by mutexes, ensuring exclusive access and preventing race conditions. It also has several tasks dependent upon it, which will be described in detail below.
- **Record** - The record task relies on the display update to tell it which track it's currently on, which tracks are currently active, as well as the playback and recording states. It also relies on the decode block to update the latest key recieved. These dependencies use shared state variables guarded by mutexes, ensuring thread-safe access to critical data.
- **Signal Generator** - The signal generator is dependent upon the display update task to select waves, and also on the decode task to assign the active voices of each note played. It's also dependent upon a sempahore from the DAC to notify it when the buffer is available to write to.

## Deadlock and Analysis
Deadlock occurs when a set of tasks forms a circular wait, where each task holds a resource and waits for another, leading to perpetual blocking. In our system, the dependency diagram suggests a potential loop involving the Recording, Transmit, Decode, and Update Display tasks. However, this loop does not result in deadlock for several reasons:

- **Synchronization Design** :
Dependencies are managed using FreeRTOS queues and mutexes. Queues allow tasks like Transmit and Decode to wait for data without holding resources, breaking the "hold and wait" condition required for deadlock. Mutexes, used in tasks like Update Display and Record, are acquired and released within the same thread, ensuring that a task cannot wait indefinitely while holding a mutex.

- **Priority Inheritance** : 
FreeRTOS’s priority inheritance mechanism further reduces risks. If a lower-priority task (e.g., Update Display) holds a mutex needed by a higher-priority task (e.g., Record), its priority is temporarily elevated to match the waiting task’s priority. This ensures the mutex is released promptly, minimizing delays and preventing priority inversion from escalating into deadlock.

- **Theoretical Deadlock Scenarios** :
Deadlock could theoretically occur if two tasks each held a mutex and waited for the other to release it, forming a circular wait. However, our system avoids this by ensuring tasks do not hold multiple mutexes simultaneously.


In summary, our system features several inter-task dependencies, primarily managed through FreeRTOS queues, mutexes, and semaphores. While a potential dependency loop exists, the use of queues for asynchronous communication prevents tasks from holding resources while waiting, and mutexes ensure resources are released within the same thread. Combined with FreeRTOS’s priority inheritance, which mitigates priority inversion, these mechanisms prevent deadlock. Thus, the system is designed to operate reliably in real-time without deadlock risks.
