# ELEC60013 - Embedded Systems: SEEEnthesizer

  This report includes:

  1. [Task Description](doc/task_description.md)
  2. [Initiation Interval & Maximum Execution Time Analysis](doc/ii_met.md)
  3. [Critical Instant Analysis](doc/cia.md)
  4. [CPU Total Utilisation](doc/cpu_util.md)
  5. [Shared Data and Methods](doc/shared_data.md)
  6. [Deadlock Analysis](doc/deadlock.md)

Written with blood, sweat, and tears by Zakariyyaa Chachia, Hector Oga, and Lolézio Viora Marquet

## Manual of Operation

### Startup

On startup the user will see the Home screen below, displaying information about the current configured octave, current key pressed, current board volume, and its internal state as "Slave" or "Master".

INSERT IMAGE

### Master / Slave Configuration

Master boards receive messages from other boards and play their own notes, as well as notes sent by other Slaves.
Slaves cannot play their own notes, and send them to the configured Master as soon as they are played.
By default, a lone keyboard is configured as a Master on startup. Also, the leftmost keyboard is configured to be a Master by default.

If the user has access to the source files, the Master/Slave configuration can be set to be manually changed by toggling the `AUTOSLAVE` flag in `include/globals.h`. If this setting is enabled on compilation, the user may click the second knob from the right to toggle the board's state. We recommend caution using this as an all-Slaves configuration may result in the boards having to be reset, as they will never get a message acknowledgement from their Master.
