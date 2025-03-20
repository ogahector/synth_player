# ELEC60013 - Embedded Systems: SEEEnthesizer
_Embedding our system_

  This report includes:

  1. [Task Description](doc/task_description.md)
  2. [Initiation Interval & Maximum Execution Time Analysis](doc/ii_met.md)
  3. [Critical Instant Analysis](doc/cia.md)
  4. [CPU Total Utilisation](doc/cpu_util.md)
  5. [Shared Data and Methods](doc/shared_data.md)
  6. [Deadlock Analysis](doc/deadlock.md)

Written with blood, sweat, and tears by Zakariyyaa Chachia, Hector Oga, and Lolézio Viora Marquet

A full video demonstration of all the implemented functionality can be seen at the following [link](https://youtu.be/jy3lUqT4a4Y): https://youtu.be/jy3lUqT4a4Y

<!-- This is more like a guideline, it doesnt matter that there's 2 # sections it's fine -->
# Manual of Operation

## Startup

On startup the user will see the Home screen below, displaying information about the current configured octave, current key pressed, current board volume, and its internal state as "Slave" or "Master".

![Home Screen](/Images/home_display.jpg)

## Master / Slave Configuration

Master boards receive messages from other boards and play their own notes, as well as notes sent by other Slaves.
Slaves cannot play their own notes, and sends them to the configured Master as soon as they are played.
By default, a lone keyboard is configured as a Master. Also, the leftmost keyboard is configured to be a Master by default.[^1]

## Volume

Only Master boards may change their own volume. The volume on a Slave will be asserted to 0 until it becomes a Master, but its notes will still get played as normal on the Master's speaker.
If the rightmost knob on a Master board is pressed, the board will be muted and an `X` will appear instead of the current volume level.

## Setting the Octave

Each board, Slave or Master may set up their own octave to play. This is to allow for complete customisation of the harmonies achievable.

## Screen Displays & Options

The Master Board's Menu can be accessed by pressing on the leftmost knob: it will show the options below. To limit customisation to the Master, a Slave's menu is more restricted and will only be able to access the [DOOM](/doc/doom.md) easter egg.

### **[MENU](/doc/menu.md)**

Displays the DOOM icon initially, which can be clicked (using the joystick) to enter Doom. When the joystick is moved left or right, the next icon slides in to take the place of the old one. Allows to select between `Doom`, `Wave Select` and `Recording`.

### **WAVE SELECT**

Displays a grid of 4 different waveforms. The user can choose between a Sine wave, Square wave, Sawtooth wave, or a Triangle wave. By default, the board starts with a Sawtooth wave on startup.

![wave_select](/Images/wave_selection.jpg)

### **RECORD**

The first screen displays a grid of 4 tracks to select. The second screen shows the selected track, button to record or play, and a back button. Navigation is done with the joystick as in the Menu part. When record or play are pressed, appropriate icons show what was selected.
If a track is playing, it will not be detect by another recording. At most the user can record up to 5 seconds of playback. If the user is playing any keys as a recording ends itself automatically, the key will be automatically released. This also serves as an end-of-recording indicator to the user.

![track_selection](/Images/Track%20Selection.jpg)
![track_inner_screen](/Images/recording_inner_screen.jpg)

### **[DOOM](/doc/doom.md)**

Renders our own version of the 1993 popular video game Doom. It displays enemies, obstacles (trees), and bullet animation, in a 2.5D rendered world (forward/backward and left/right movement). It also displays the frames per second (FPS) of the gameplay, to demonstrate the real-time performance of this system.

## Good Software Engineering Practices

In this project, we undertook the task of working as a team from different academic backgrounds, not necessarily software focused. To ensure team communication and progressing as engineers, we adhered to the following coding conventions and rules of cooperation - some of these include pragmatic coding practices, others are guidelines.

1. Consistent naming convention
2. Separate each Task into its own file for readability
3. Readability over raw performance, when possible
4. HAL over Arduino built-in libraries, except where performance gain is minimal
5. HAL over raw flag access for readability, unless performance is in question
6. Never pushing to production unless code has been verified / tested
7. Work on separate tasks when working from home - Work on similar tasks in person
8. Short and concise interrupts - they must **never** be blocking
9. No dynamic memory allocation, unless to safeguard the system, in which case the expected max size must be preallocated.
10. Regular code reviews to ensure code quality and knowledge sharing

[^1]: If the user has access to the source files, the Master/Slave configuration can be set to be manually changed by toggling the `AUTOSLAVE` flag in `include/globals.h`. If this setting is enabled on compilation, the user may click the second knob from the right to toggle the board's state. We recommend caution using this as an all-Slaves configuration may result in the boards having to be reset, as they will never get a message acknowledgement from their Master.
