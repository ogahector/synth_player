//Doom source file
#include <globals.h>
#include <doom.h>
#include <doom_def.h>

// Show DOOM loading screen
bool doomLoadingShown = false;

// Define bullet properties
#define MAX_BULLETS 10 // Max number of bullets on screen

struct Bullet {
    int x, y;
    bool active;  // Whether the bullet is currently active
};

Bullet bullets[MAX_BULLETS];  // Array of bullets

// Initialize bullets
void initBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;  // Set all bullets as inactive initially
    }
}

// Shoot a bullet from the center of the gun
void shootBullet() {
    // Find an inactive bullet slot
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = 69;   // Start from center-bottom of the screen (adjust if needed)
            bullets[i].y = 19;   // Just above the gun
            bullets[i].active = true;
            break;
        }
    }
}

// Move and render bullets
void updateBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].y -= 1;  // Move bullet upwards (adjust speed if needed)
            bullets[i].x -= 1;
            // Check if the bullet has moved off-screen
            if (bullets[i].y < 0) {
                bullets[i].active = false;  // Deactivate the bullet
            } else {
                // Draw the bullet as a pixel
                u8g2.drawPixel(bullets[i].x, bullets[i].y);
            }
        }
    }
}


void renderDoomScene() {
    // Only show loading screen once per activation of doom mode
    if (!doomLoadingShown) {
      u8g2.clearBuffer();
      u8g2.setDrawColor(1);  // 1 = white, 0 = black
  
      // Render the loading image (from doomLoadScreen)
      for (size_t i = 0; i < numLoadOnes; i++) {
        u8g2.drawPixel(doomLoadScreen[i].col, doomLoadScreen[i].row);
      }
      u8g2.sendBuffer();
  
      // Wait for 1 second
      vTaskDelay(1000 / portTICK_PERIOD_MS);
  
      // Mark the loading screen as shown
      doomLoadingShown = true;
    }
  
    // Always display the start scene
    u8g2.clearBuffer();
    for (size_t i = 0; i < numGun; i++) {
      u8g2.drawPixel(doomGun[i].col+10, doomGun[i].row);
    }

    // Shoot when joystick is pressed
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    bool shoot= sysState.joystickPress;
    xSemaphoreGive(sysState.mutex);
    if (shoot) {
      shootBullet();
    }

    // Update and render bullets
    updateBullets();
    // for (size_t i = 0; i < numEnemy; i++) {
    //   u8g2.drawPixel(doomEnemy[i].col-30, doomEnemy[i].row);
    // }
    u8g2.sendBuffer();
  }