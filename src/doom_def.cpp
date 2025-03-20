//Doom source file
#include <globals.h>
#include <doom_def.h>
#include <vector>
#include <unordered_map>

const int projCenterX = 64; // assuming a 128x32 display
const int projCenterY = 16;

bool showGun=true;

float playerX = 0;
float playerZ = 0;

bool killedMonster = false;
uint8_t count=0;

float focalLength = 50.0;
int score=0;

unsigned long lastFrameTime = 0;
float fps = 0.0;
int frameCount = 0;
unsigned long lastFpsUpdate = 0;


// ###### CHUNK LOGIC ######
int chunkSize = 100;
const int renderDistanceSide = 1; // Side render distance
const int renderDistanceForward = 2; // More forward chunks

#define MAX_CHUNKS (2*renderDistanceSide +1) * (renderDistanceForward +1)

// ####### BULLET LOGIC ########
// Define bullet properties
#define MAX_BULLETS 1 // Max number of bullets on screen

// Now bullets are defined in world coordinates, with a velocity.
struct Bullet {
    float worldX, worldZ, worldY; // World coordinates (X = lateral, Z = depth)
    float vX, vZ, vY;         // Velocity in world coordinates
    bool active;           // Whether the bullet is active
};

Bullet bullets[MAX_BULLETS];  // Array of bullets

// Initialize bullets: mark all as inactive.
void initBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }
}

// Shoot a bullet from the gun.
// This function sets the bullet's initial world coordinates relative to the player
// and assigns a velocity so that it moves forward.
void shootBullet() {
    // uint8_t releaseEvent[8] = {'P', 0, 3, 8, 0, 0, 0, 0};
    // xSemaphoreTake(sysState.mutex,portMAX_DELAY);
    // if (sysState.slave) xQueueSend(msgOutQ, releaseEvent, portMAX_DELAY);
    // else xQueueSend(msgInQ,releaseEvent,portMAX_DELAY);
    // xSemaphoreGive(sysState.mutex);
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            // Assume the bullet originates at the player's gun position.
            // Adjust these offsets as needed.
            bullets[i].worldX = playerX;      // bullet starts at player's lateral position
            bullets[i].worldZ = playerZ + 5;    // bullet starts 5 units in front of the player
            bullets[i].worldY = 19; 
            // Set the bullet velocity (e.g., moving straight forward)
            bullets[i].vX = -1;    // No lateral movement
            bullets[i].vZ = 3;    // Moves forward 2 units per update (adjust speed as needed)
            bullets[i].vY = -0.5;
            bullets[i].active = true;
            break;
        }
    }
}

// Update and render bullets using world coordinates.
void updateBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            // Update bullet position in world coordinates.
            bullets[i].worldX += bullets[i].vX;
            bullets[i].worldZ += bullets[i].vZ;
            bullets[i].worldY += bullets[i].vY;

            // Optionally, check if the bullet is too far (e.g., off the map).
            // For example, if the bullet has moved too far forward, deactivate it:
            if (bullets[i].worldZ > playerZ + 50) { // adjust threshold as needed
                bullets[i].active = false;
                continue;
            }

            // Calculate differences relative to the player.
            float dx = bullets[i].worldX - playerX;
            float dz = bullets[i].worldZ - playerZ;
            float dy = bullets[i].worldY - 19;
            // Only render bullets that are in front of the player.
            if (dz <= 0) {
                bullets[i].active = false;
                continue;
            }
            // Compute distance and scale.
            float distance = std::sqrt(dx * dx + dz * dz);
            if (distance < 1) distance = 1;  // Avoid division by zero.
            float scale = focalLength / distance;

            // A simple projection: scale the difference and add to screen center.
            int screenX = 69 + static_cast<int>(dx * scale);
            int screenY = 19 + static_cast<int>(dy * scale);;
            
            // Draw the bullet as a pixel.
            u8g2.drawPixel(screenX, screenY);
        }
    }
}

bool pointCollides(int objX, int objZ, int pointX, int pointZ, float baseRadius) {
  int dx = objX - pointX;
  int dz = objZ - pointZ;
  return (dx * dx + dz * dz < baseRadius*baseRadius);
}

// ########### ENEMY LOGIC ##########
class Enemy {
  public:
    int worldX, worldZ; // Enemy's world coordinates (x = lateral, z = depth)
    bool active;
    int centerX=60;
    int centerY=16;
    float scale;

    Enemy() : worldX(0), worldZ(0), active(false) {}
    // Constructor: world coordinates determine the enemy's initial placement.
    Enemy(int wx, int wz)
      : worldX(wx), worldZ(wz), active(true) {}
  
    void render() {
      float dx = worldX - playerX;  // lateral difference from player
      float dz = worldZ - playerZ;  // depth difference from player
  
      // Only render if the enemy is in front of the player.
      if (dz <= 0) return;
  
      // Compute distance from the player and derive a scale factor.
      float distance = std::sqrt(dx * dx + dz * dz);
      if (distance < 1) distance = 1;  // prevent division by zero
      scale = focalLength / distance;
  
      // Compute the enemy's projected anchor position on screen.
      float angleShift = atan2(dx, dz);  // Angle between player and enemy
      float projectedX = projCenterX + tan(angleShift) * focalLength;
      int projectedY = 15; // Fixed vertical position (adjust if you use vertical world coordinates)
  
      // Define a base sprite size. Increase this value to start with a larger image.
      int baseSpriteSize = 5;  // Increased from 10 for a larger starting size
  
      // Now render the enemy sprite. We assume doomEnemy[0] is the anchor.
      for (size_t i = 0; i < numEnemy; i++) {
        // Get the local coordinates of this pixel in the sprite.
        int spriteX = doomEnemy[i].col;
        int spriteY = doomEnemy[i].row;
        // Calculate the offset from the sprite's anchor (doomEnemy[0]).
        int localOffsetX = spriteX - centerX;
        int localOffsetY = spriteY - centerY;
        // Scale the local offsets.
        int renderX = projectedX + static_cast<int>(localOffsetX * scale);
        int renderY = projectedY + static_cast<int>(localOffsetY * scale);
        // Draw the pixel.
        u8g2.drawPixel(renderX, renderY);
      }
    }
    bool collidesWithPoint(int x, int z, bool playerCollision) {
      if (playerCollision){
        return pointCollides(worldX, worldZ, x, z, 10);
      }
      else{
        return pointCollides(worldX, worldZ, x, z, 15);
      }

      
    }
};

// ########### OBSTACLE LOGIC ##########
class Obstacle {
  public:
    int worldX, worldZ; 
    bool active;
    int centerX = 64;
    int centerY = 17;
    float scale;
    
    Obstacle() : worldX(0), worldZ(0), active(false) {}
    // Constructor: world coordinates determine the obstacle's initial placement.
    Obstacle(int wx, int wz)
      : worldX(wx), worldZ(wz), active(true){}
    
    void render() {
      float dx = worldX - playerX;  // lateral difference from player
      float dz = worldZ - playerZ;  // depth difference from player
  
      // Only render if the enemy is in front of the player.
      if (dz <= 0) return;
  
      // Compute distance from the player and derive a scale factor.
      float distance = std::sqrt(dx * dx + dz * dz);
      if (distance < 1) distance = 1;  // prevent division by zero
      scale = focalLength / distance;
  
      // Compute the enemy's projected anchor position on screen.
      float angleShift = atan2(dx, dz);  // Angle between player and enemy
      float projectedX = projCenterX + tan(angleShift) * focalLength;
      int projectedY = 15; // Fixed vertical position (adjust if you use vertical world coordinates)
  
      // Define a base sprite size. Increase this value to start with a larger image.
      int baseSpriteSize = 20;  // Increased from 10 for a larger starting size
  
      // Now render the enemy sprite. We assume doomEnemy[0] is the anchor.
      for (size_t i = 0; i < numObstacle; i++) {
        // Get the local coordinates of this pixel in the sprite.
        int spriteX = obstacle[i].col;
        int spriteY = obstacle[i].row;
        // Calculate the offset from the sprite's anchor (doomEnemy[0]).
        int localOffsetX = spriteX - centerX;
        int localOffsetY = spriteY - centerY;
        // Scale the local offsets.
        int renderX = projectedX + static_cast<int>(localOffsetX * scale);
        int renderY = projectedY + static_cast<int>(localOffsetY * scale);
        // Draw the pixel.
        u8g2.drawPixel(renderX, renderY);
      }
    }
  
    bool collidesWithPoint(int x, int z) {
      return pointCollides(worldX, worldZ, x, z, 5);
    }
  };

struct ChunkData {
  Enemy enemy;
  Obstacle obstacle;
  bool enemyActive = false;
  bool obstacleActive = false;
  bool isGenerated=false;
  int chunkX;
  int chunkZ;
};
  
ChunkData chunkStorage[MAX_CHUNKS];

// ######## CHUNK GENERATION LOGIC #########
void generateChunk(int chunkX, int chunkZ) {
  for (int i = 0; i < MAX_CHUNKS; i++) {
    if (!chunkStorage[i].isGenerated) {
        // Create enemies and obstacles for this chunk
        int enemyX = chunkX * chunkSize + random(0, chunkSize-10);
        int enemyZ = chunkZ * chunkSize + random(0, chunkSize-10);
        chunkStorage[i].enemy = Enemy(enemyX, enemyZ);
        chunkStorage[i].enemyActive = true;

        int obstacleX = chunkX * chunkSize + random(-chunkSize / 2, chunkSize / 2);
        int obstacleZ = chunkZ * chunkSize + random(-chunkSize / 2, chunkSize / 2);
        chunkStorage[i].obstacle = Obstacle(obstacleX, obstacleZ);
        chunkStorage[i].obstacleActive = true;
        chunkStorage[i].isGenerated = true;
        chunkStorage[i].chunkZ=chunkZ;
        chunkStorage[i].chunkX=chunkX;
      break;
    }
  }
}

void cleanupOldObjects(int playerChunkX, int playerChunkZ) {
  for (int i = 0; i < MAX_CHUNKS; i++) {
    if (!chunkStorage[i].isGenerated) continue;
    int dx = chunkStorage[i].chunkX - playerChunkX;
    int dz = chunkStorage[i].chunkZ - playerChunkZ;
    if (abs(dx) > renderDistanceSide || dz > renderDistanceForward || dz < 0) {
      chunkStorage[i].isGenerated = false;
    } 
  }
}

void updateWorld() {
  int playerChunkX = playerX / chunkSize;
  int playerChunkZ = playerZ / chunkSize;
  // Generate nearby chunks
  for (int x = -renderDistanceSide; x <= renderDistanceSide; x++) {
    for (int z = 0; z <= renderDistanceForward; z++) {
      generateChunk(playerChunkX + x, playerChunkZ + z);
    }
  }

  // Render all active objects in active chunks
  for (int i = 0; i < MAX_CHUNKS; i++) {
      if (!chunkStorage[i].isGenerated) continue;
      if (chunkStorage[i].enemyActive) {
        chunkStorage[i].enemy.render();
      }
      if (chunkStorage[i].obstacleActive) {
        chunkStorage[i].obstacle.render();
    }
  }
  // Clean up old chunks
  cleanupOldObjects(playerChunkX, playerChunkZ);
}

void checkCollisions() {
  // Bullet collisions
  for (int i = 0; i < MAX_BULLETS; i++) {
      if (!bullets[i].active)
          continue;

      // Check collision with obstacles
      for (int c = 0; c < MAX_CHUNKS; c++) {
          if (!chunkStorage[c].isGenerated) continue;
          if (chunkStorage[c].obstacleActive && chunkStorage[c].obstacle.collidesWithPoint(bullets[i].worldX, bullets[i].worldZ)) {
            bullets[i].active = false;
            break;
          }
        if (!bullets[i].active) break;

        if (chunkStorage[c].enemyActive && chunkStorage[c].enemy.collidesWithPoint(bullets[i].worldX, bullets[i].worldZ, false)) {
          uint8_t releaseEvent[8] = {'P', 4, 1, 8, 0, 0, 0, 0};
          xSemaphoreTake(sysState.mutex,portMAX_DELAY);
          if (sysState.slave) xQueueSend(msgOutQ, releaseEvent, portMAX_DELAY);
          else xQueueSend(msgInQ,releaseEvent,portMAX_DELAY);
          xSemaphoreGive(sysState.mutex);
          score += 100;                       // Increase score
          bullets[i].active = false;          // Deactivate bullet
          chunkStorage[c].enemyActive = false; // Mark enemy as inactive
          killedMonster=true;
          break;
        }
      }
      if (!bullets[i].active) break;  // Stop further checks if bullet is gone
  }

  // Enemy-player collisions
  for (int c = 0; c < MAX_CHUNKS; c++) {
      if (!chunkStorage[c].isGenerated) continue; // Skip inactive chunks
      if (chunkStorage[c].enemyActive && chunkStorage[c].enemy.collidesWithPoint(playerX, playerZ, true)) {
        // Game over logic
        u8g2.clearBuffer();
        for (int i = 0; i < numGameOver; i++) {
            u8g2.drawPixel(gameOver[i].col, gameOver[i].row);
        }
        u8g2.sendBuffer();
        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        sysState.activityList = MENU;
        xSemaphoreGive(sysState.mutex);
        #ifndef TEST_DISPLAYUPDATE
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        #endif
        showGun = false;
        return;  // No need to check further, game over!
      }
  }
}

void updatePlayerPosition(int jx, int jy) {
  float newPlayerX = playerX;
  float newPlayerZ = playerZ;
  if (jx > JOYX_THRESHOLD_LEFT_DOOM || jx < JOYX_THRESHOLD_RIGHT_DOOM) {
      newPlayerX -= (jx - 747) * 0.005;
  }
  if (jy < JOYY_THRESHOLD_UP_DOOM || jy > JOYY_THRESHOLD_DOWN_DOOM) {
      newPlayerZ += (482 - jy) * 0.005;
  }
  #ifdef TEST_DISPLAYUPDATE
  newPlayerX += (800 - 460) * 0.005;
  newPlayerZ += (516 - 800) * 0.005;
  #endif
  bool collision = false;
  for (int c = 0; c < MAX_CHUNKS; c++) {
    if (!chunkStorage[c].isGenerated) continue;
    if (chunkStorage[c].obstacleActive &&chunkStorage[c].obstacle.collidesWithPoint(newPlayerX, newPlayerZ)) {
      collision = true;
      if (newPlayerX != playerX) {
        playerX = playerX;  
      }
      if (newPlayerZ != playerZ) {
        playerZ = playerZ; 
      }
      break;
    }
    if (collision) break;
  }
  if (!collision) {
    playerX = newPlayerX;
    playerZ = newPlayerZ;
  }
}

// ######### MAIN LOOP ##########
void renderDoomScene(bool doomLoadingShown) {
  // Only show loading screen once per activation of doom mode
  if (!doomLoadingShown) {
    u8g2.clearBuffer();
    u8g2.setDrawColor(1);
    // Render the loading image (from doomLoadScreen)
    for (size_t i = 0; i < numLoadOnes; i++) {
      u8g2.drawPixel(doomLoadScreen[i].col, doomLoadScreen[i].row);
    }
    u8g2.sendBuffer();
    for (int c = 0; c < MAX_CHUNKS; c++) {
      chunkStorage[c].obstacleActive=false;
      chunkStorage[c].enemyActive=false;
      chunkStorage[c].isGenerated=false;
    }
    playerX=chunkSize;
    playerZ=chunkSize;
    showGun=true;
    score=0;
    doomLoadingShown=true;
    #ifndef TEST_DISPLAYUPDATE
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    #endif
  }
  u8g2.clearBuffer();
  
  updateWorld();
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

  updatePlayerPosition(jx, jy);
  
  u8g2.setCursor(100,10);
  u8g2.print(score, DEC);
  
  checkCollisions();

  if(showGun){
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
    frameCount++;
    unsigned long currentTime = micros();
    if (currentTime - lastFpsUpdate >= 1000) { // Update every 1 second
      fps = frameCount / ((currentTime - lastFpsUpdate) / 1000000.0);
      frameCount = 0;
      lastFpsUpdate = currentTime;
    }
    u8g2.setCursor(0, 10); // Top-left corner (adjust position as needed)
    u8g2.print("FPS: ");
    u8g2.print(fps, 1);
  }
  
  if (killedMonster){
    count++;
    if (count==20){
      count=0;
      killedMonster=false;
      uint8_t releaseEvent[8] = {'R', 4, 1, 8, 0, 0, 0, 0};
      xSemaphoreTake(sysState.mutex,portMAX_DELAY);
      if (sysState.slave) xQueueSend(msgOutQ, releaseEvent, portMAX_DELAY);
      else xQueueSend(msgInQ,releaseEvent,portMAX_DELAY);
      xSemaphoreGive(sysState.mutex);
    }
  }
  
  u8g2.sendBuffer();
}