# DOOM

As an advanced functionality, we decided to attempt to push the board and the joystick to its limit. In recent years, porting the highly optimised 1993 video game Doom has been an interesting exercise, as can be seen on the website [CanItRunDoom.org](https://canitrundoom.org/).

However, the smallest possible port of the game is 1.4MB, due to the texture packs and other game files. 

Therefore, we had to implement our own version of the game. The version running on this system can be seen as an emulation of the game, as a proof of concept.

## Thread Safety
Due to the system wide state machine, the DOOM state only renders the Doom gameplay on the displayUpdateTask, making all game logic run in a self contained fashion. The game controls, however, are run in a separate thread, which makes the game more responsive. This is done in the Scan Keys task, updating the ```sysState``` global variable with the latest joystick data, but only when taking the mutex. The display Update thread then reads this data, also by taking the mutex. 

## Loading game data
The sprites and the environment from the original game had to be ported to the black and white, 32x128 screen, which is a significant resolution reduction from the original game. 

The Python script [doom.py](doom/doom.py) contains the script written to extract game data. 

It takes in normal, coloured images from the game, and applies binary threshholding to extract a black and white image. Then, the image is resized to the correct dimensions. 

Earlier methods used Canny edge detection and scaling parameters, which did not prove effective in extracting the correct image due to noise. Manually adjusting the treshhold proved quite effective and efficient.

These images are then stored as (row,column) pairs, where a pair corresponds to a white pixel on the screen. These can be stored in a C header file, loaded into memory. 

For Doom, the total list of assets being loaded are as follows:
- Doom Loading Screen
    - Loading screen that shows when the Doom option is selected
- Obstacle
    - Tree sprite to act as an obstacle in the game
- Game Over
    - Game Over screen when the player is killed
- Gun
    - The gun held by the player, to show the location of the player on screen
- Gun Outline
    - Negative of the player's gun, so that an outline can be drawn and fact the gun is in front of objects can be seen
        - This is not calculated at runtime as this would decrease performance too much, and loading the data from memory proved more efficient
- Doom Enemy
    - Sprite of one of the original enemies from the Doom game

## Game Engine
Doom is a first person shooter game, which means that the player does not move on screen, but rather the entire environment is scaled to perspective to emulate movement. The player's gun remains centralised in the middle of the screen. The player also cannot jump, but rather move side to side, forward and backward.

### World and screen coordinates
The game's world is defined as an x-z plane, where x is sideways movement, and z is how much the player has moved forward or backward. This allows to calculate the distance between objects (enemies, obstacles and bullets) to the player, which must then be mapped to x-y plane coordinates, to be drawn as pixels on the output screen. 

The x-z coordinates can be used to calculate the Euclidean distance between the player and an object's x-z coordinates. Then, this distance is used to calculate the scale of the objects, according to the formula $ scale = \frac{focal\_lense}{distance} $, where $ focal\_lense $ is manually adjusted to achieve the desired field of view. 

The object's position on the screen can then be mapped by the angle to the player's forward facing field of view, according to the formula: $ projectedX = centerX + \tan(\frac{x}{z}) * focal\_lense $.

Then, the sprite can be plotted, as this projectedX value + its own pixel coordinates scaled, since we have a mapping from the x-z plane to the x-y plane.


### World Building
The enemies and obstacles are each stored as a class in the code, with a ```render()``` function that calculates the offsets explained in the previous section based on the player's new position.

This is so multiple versions of each can be randomly generated at different positions. To avoid overusing memory, a chunk based generation system is implemented.

The world is split in 100x100 chunks that each contain 1 enemy and 1 obstacle. Each one can be uniquely identified, and has a center value. It is generated when its center value enters the Euclidean render distance of the player, and deallocated when it is no longer within the render distance. This ensures performance remains more or less the same as the player moves through the world.

It also makes the gameplay much smoother, as it loads enemies and obstacles far from the player, as it progresses towards them. 

The ```renderDistance``` parameter also allows whether to optimise for gameplay or performance. With a higher render distance, the gameplay is much smoother and the world is richer. However, this decreases the framerate, since more objects have to be scaled on each frame.

An average framerate of around 25FPS was achieved with ```renderDistance``` set to 3. However, setting it to just 1, meaning only the front and side chunks as well as the player's current chunks will be loaded, can significantly boost performance.

### Player Movement
The player movement depends on the read joystick value. The location of the player in the x-z plane is stored as a global variable, so it can be updated depending on the measured joystick value (scaled). A linear subtraction is used, so that the movement speed scales linearly with the read joystick value. This allows for finer control of movement speed. Thresholding of the joystick values is used to prevent joystick drift from moving the player.

Before the global player location is updated, a collision with an obstacle is checked to ensure that the player cannot move through obstacles. 

### Gun control
When the joystick is pressed, the gun fires a bullet, which does not toggle, meaning more bullets can be fired, for a maximum of 5 at a given time. The state of these bullets is stored in an array, as their respective x, y, z positions must be known for collision detection. 

Since they move independently, their location in world (x-z) coordinates is updated at every frame, which is then mapped to the screen's x-y coordinate frame.

### Collisions
Collisions are defined by the Euclidean distance (in x-z) of the two objects to be below a certain threshhold (hitbox dimensions). The hitbox dimension depends on each object. 

4 types of collisions are introduced in the game:
- Bullet/Obstacle Collision
    - When a bullet is fired, at every frame the game engine checks whether it is within hitbox distance of an obstacle. If it is, the bullet disappears
- Bullet/Enemy Collision
    - As above, but this causes the enemy to die, increments score, and plays a random note.
    - The sound is written to the queue, so that it can be efficiently played, without blocking. 
- Player/Obstacle Collision
    - This simply prevents the player from moving in the direction it is currently moving in, forcing them to go around the obstacle.
- Player/Enemy Collision
    - This causes the player to lose the game, and triggers the Game Over screen

## Optimisations carried out
A number of optimisations and revisions were carried out to the game engine to improve performance. 

The original version of the game took over 80ms to refresh a new frame, which was critically too slow, and made for poor gameplay.

A significant speedup carried out was switching between the ```double``` and ```float``` data types for floating point arithmetic. Since precision is not important in this case (as the final output will always be integer coordinates), the ```float``` data type is much more appropriate.

Next, expensive mathematical operations such as square roots were minimsed as much as possible. By simply using squared distance instead of actual distance, the same result can be achieved, at a much lower computational cost. 

The initial implementation also dynamically stored chunks, enemies and obstacles in ```std::vector``` data types. Dynamically allocating and unallocating proved to introduce some overhead, and potentially could have resulted in stack overflows if the deallocating failed. To improve this, a fixed render distance is set at compile time, and so a fixed number of chunks is set. Therefore, a preallocated array can be used, which activates and deactivates chunks as they enter and leave the render distance. This allows for chunk generation in all directions around the player, and as soon as a chunk is within render distance, logically a chunk in the opposite direction would be outside the distance, and be deactivated. Its coordinates are therefore updated with the new chunk coordinates, and new enemies and obstacle objects are created for it.