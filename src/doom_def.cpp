//Doom source file
#include <globals.h>
#include <doom_def.h>

// Define bullet properties
#define MAX_BULLETS 10 // Max number of bullets on screen

static int cameraOffsetX = 0;
static int cameraOffsetY = 0;

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

static double previousScale = 0.5;
bool loss=false;
int verticalDelta = 0;
double maxScaleChangePerFrame = 0.05; 

void renderDoomScene(bool doomLoadingShown) {
    
    // Only show loading screen once per activation of doom mode
  if (!doomLoadingShown) {
    u8g2.clearBuffer();
    u8g2.setDrawColor(1);  // 1 = white, 0 = black
  
      // Render the loading image (from doomLoadScreen)
    for (size_t i = 0; i < numLoadOnes; i++) {
      u8g2.drawPixel(doomLoadScreen[i].col, doomLoadScreen[i].row);
    }
    u8g2.sendBuffer();
    cameraOffsetX = 0;
    cameraOffsetY = 0;
    loss=false;
      // Wait for 1 second
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  u8g2.clearBuffer();
    
  int jx, jy;
  bool shoot;
  xSemaphoreTake(sysState.mutex, portMAX_DELAY);
  jx = sysState.joystickHorizontalDirection;
  jy = sysState.joystickVerticalDirection;
  shoot = sysState.joystickPress;
  xSemaphoreGive(sysState.mutex);
  if (shoot) {
    shootBullet();
  }

  // Update and render bullets
  updateBullets();
  
  
  // Define a center of projection.
  const int projCenterX = 64; // assuming a 128x32 display
  const int projCenterY = 16;

  // Determine a scaling factor based on the joystick vertical reading.
  // For example, assume that the joystick’s "neutral" vertical value is 516.
  // When the joystick goes up (value < 516), we simulate moving forward by increasing the scale.
  
  if (jy<400 || jy>600){
    verticalDelta = 516 - jy;
    double targetScale = 0.5 + static_cast<double>(verticalDelta) / 200.0;

  // Limit the rate of change in scale (prevents sudden zooms)
  
    if (targetScale > previousScale + maxScaleChangePerFrame) {
      targetScale = previousScale + maxScaleChangePerFrame;  // Slow down zoom-in
    }
    if (targetScale < previousScale - maxScaleChangePerFrame) {
      targetScale = previousScale - maxScaleChangePerFrame;  // Slow down zoom-out
    }
    previousScale = targetScale;
  }
  
  if (jx>600 || jx<400){
    cameraOffsetX += ((jx - 460) / 150) * previousScale;
  }

  if (previousScale<0) previousScale=0;
  if (previousScale>4) {
    previousScale=4;
  }
  
    for (size_t i = 0; i < numEnemy; i++) {
      // Get world coordinates for this enemy pixel.
      int worldX = doomEnemy[i].col;
      int worldY = doomEnemy[i].row;
      // Compute displacement from the projection center.
      int dx = worldX - projCenterX;
      int dy = worldY - projCenterY;
      // Apply scaling to simulate moving forward.
      double fx = dx * previousScale;
      double fy = dy * previousScale;
      int projectedX = projCenterX + static_cast<int>(fx) + cameraOffsetX;
      int projectedY = projCenterY + static_cast<int>(fy);

      bool enemyCentered = (abs(projectedX - projCenterX) < 200);  // Enemy is near center
      bool enemyTooClose = (previousScale>3.9);  // Enemy is "big enough"

      if (enemyCentered && enemyTooClose) {
        loss=true;
      }
      
      int pixelSize = static_cast<int>(previousScale); 
      if (pixelSize < 1) pixelSize = 1;

      // Fill in the area to prevent gaps
      for (int x = 0; x < pixelSize; x++) {
        for (int y = 0; y < pixelSize; y++) {
            u8g2.drawPixel(projectedX + x, projectedY + y);
        }
      }
  }
  Serial.println(loss);
  if (loss){
    u8g2.clearBuffer();
    for (int i = 0; i < numGameOver; i++) {
      u8g2.drawPixel(gameOver[i].col, gameOver[i].row);
    }
    u8g2.sendBuffer();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    sysState.activityList = HOME;
    xSemaphoreGive(sysState.mutex);
  }


  // Draw the gun
  u8g2.setDrawColor(0);
  for (size_t i = 0; i < numGunOutline; i++) {
      u8g2.drawPixel(gunOutline[i].col + 10, gunOutline[i].row); // This uses the current draw color (assumed white)
  }
  u8g2.setDrawColor(1);

  // Then draw the gun pixel itself (which will not remove the outline)
  for (size_t i = 0; i < numGun; i++) {
    u8g2.drawPixel(doomGun[i].col + 10, doomGun[i].row); // This uses the current draw color (assumed white)
  }
  u8g2.sendBuffer();
}