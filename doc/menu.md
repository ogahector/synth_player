# MENU Logic Explanation

The MENU state displays a graphical user interface, to be able to navigate to functionality in a simple, user friendly way using the joystick.

This involves rendering three different icons, for the DOOM, wave select and recording tasks.

The joystick sliding left or right allows for the current icon to slide left or right. The joystick value is read from ```sysState```, using the mutex. 

To select a functionality, the joystick must be pressed, which changes the state of the system's state machine, again achieved by reading and writing to ```sysState```, using the mutex.

To make the menu user friendly, the new icon slide to replace the old icon. To improve the reactivity of the system, and ensure sufficient steps can be used to make it very smooth, we precalculate the transition $x$ values of the old and new icon, at compile time. This prevents from having to perform expensive trigonometric calculations for each step, and allows for very smooth display logic.

## DOOM Icon

The DOOM icon is simply the DOOM loading screen that is scaled to fit inside a rounded rectangle.
![DOOMICON](/Images/doom_icon.jpg)

## Wave Select Icon

This loads an icon from memory, that was extracted using the same technique as for the DOOM game data (see [DOOM](DOOM.md)). It then renders it within a rounded rectangle.

![WAVEICON](/Images/wave_select.jpg)

## Recording Icon

This icon is dynamically rendered, as it consists of a string and a circle. This makes it render even faster than the previous icons.

![RECORDICON](/Images/record_icon.jpg)


## Master/Slave Menu Logic

Since masters and slaves are set automatically depending on the relative position of the synthesisers, the MENU page also automatically detects which parts of the menu it needs to render. Slaves should not have access to recording and wave selection, and so this is disabled if the board is a slave (by checking ```sysState``` using the mutex). Doom remains playable on all boards.
