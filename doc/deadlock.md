# Deadlock and dependencies

## Dependencies
The dependencies we determined for our system are shown in the diagram below : 
![Dependencies](/Images/dependencies.png)

As you can see above, we have a few blocking dependencies between tasks. Below we'll discuss each of them, and their impacts:
- **Scan Keys** - Scan keys has no dependencies, but it is the source of a few different dependencies
- **Transmit** - The transmit task relies on inputs to it's queue from either the scan keys task (if the board is set up a slave), or the recording task.
- **Decode** - The decode task will also depend on inputs from it's queue to function. These inputs can come from both the transmit task (via the CAN bus), or directly from the scan keys task (if the board is configured as a master).
- **Update Display** - The update display task relies on a few different tasks to control it's output. It relies on the Scan keys task to determine if it is within the menu state, or the home state. And it also relies on the decode task to update it's message output (when in the home state). It also has several tasks dependent upon it, which will be described in detail below.
- **Record** - The record task relies on the display update to tell it which track it's currently on, which tracks are currently active, as well as the playback and recording states. It also relies on the decode block to update the latest key recieved.
- **Signal Generator** - The signal generator is dependent upon the display update task to select waves, and also on the decode task to assign the active voices of each note played. It's also dependent upon a sempahore from the DAC to notify it when the buffer is available to write to. 

## Deadlock and Analysis
Deadlock occurs when there is a dependecy loop within our system. From the diagram above, there appears to be a loop through the recording, transmit and decode blocks (also through the update display block too). This would cause an issue if each of these tasks in the loop were to enter a state of perpetual waiting for the release of the blocking task that causes the dependeny. However, the majority of the blocking here is due to mutexes, which do not cause deadlock, due to the fact that mutexes are always returned in the same thread they're taken. This removes the deadlock loop from the system, and ensure's it's dependency safe. 

In conclusion, our system has some blocking dependencies, however there are none that have the ability to deadlock the system. 