//Doom source file
#include <globals.h>
#include <doom_def.h>
#include <vector>
#include <unordered_map>

const int projCenterX = 64; // assuming a 128x32 display
const int projCenterY = 16;

bool showGun=true;

double playerX = 0;
double playerZ = -20;

double focalLength = 50.0;
int score=0;


// ###### CHUNK LOGIC ######

int chunkSize = 128;
const int renderDistanceSide = 1; // Side render distance
const int renderDistanceForward = 2; // More forward chunks

struct Chunk {
  int chunkX, chunkZ;
  bool generated;
};

// A key for uniquely identifying chunks
struct ChunkKey {
    int x, z;
    bool operator==(const ChunkKey &other) const {
        return x == other.x && z == other.z;
    }
};

// Custom hash function for ChunkKey to use in unordered_map
namespace std {
    template <>
    struct hash<ChunkKey> {
        size_t operator()(const ChunkKey &key) const {
            return hash<int>()(key.x) ^ hash<int>()(key.z);
        }
    };
}

// Store chunks in a global map instead of a list
std::unordered_map<ChunkKey, Chunk> generatedChunks;

// ####### BULLET LOGIC ########
// Define bullet properties
#define MAX_BULLETS 5 // Max number of bullets on screen

// Now bullets are defined in world coordinates, with a velocity.
struct Bullet {
    double worldX, worldZ, worldY; // World coordinates (X = lateral, Z = depth)
    double vX, vZ, vY;         // Velocity in world coordinates
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
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            // Assume the bullet originates at the player's gun position.
            // Adjust these offsets as needed.
            bullets[i].worldX = 69;      // bullet starts at player's lateral position
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

            // Convert the bullet's world coordinates to screen coordinates.
            // Calculate differences relative to the player.
            double dx = bullets[i].worldX - 69;
            double dz = bullets[i].worldZ - playerZ;
            double dy = bullets[i].worldY - 19;
            // Only render bullets that are in front of the player.
            if (dz <= 0) {
                bullets[i].active = false;
                continue;
            }
            // Compute distance and scale.
            double distance = std::sqrt(dx * dx + dz * dz);
            if (distance < 1) distance = 1;  // Avoid division by zero.
            double scale = focalLength / distance;

            // A simple projection: scale the difference and add to screen center.
            int screenX = 69 + static_cast<int>(dx * scale);
            int screenY = 19 + static_cast<int>(dy * scale);;
            
            // Draw the bullet as a pixel.
            u8g2.drawPixel(screenX, screenY);
        }
    }
}


bool pointCollides(int objX, int objZ, int pointX, int pointZ, double baseRadius, double scale) {
  int dx = objX - pointX;
  int dz = objZ - pointZ;
  double effectiveRadius = baseRadius * scale;
  return (std::sqrt(dx * dx + dz * dz) < effectiveRadius);
}


// ########### ENEMY LOGIC ##########
class Enemy {
  public:
    int worldX, worldZ; // Enemy's world coordinates (x = lateral, z = depth)
    bool active;
    bool enemyCentered;
    int centerX, centerY;
    double scale;
    
    // Constructor: world coordinates determine the enemy's initial placement.
    Enemy(int wx, int wz)
      : worldX(wx), worldZ(wz), active(true), enemyCentered(false) {
        int minX = doomEnemy[0].col, maxX = doomEnemy[0].col;
        int minY = doomEnemy[0].row, maxY = doomEnemy[0].row;
      for (size_t i = 1; i < numEnemy; i++) {
        if (doomEnemy[i].col < minX) minX = doomEnemy[i].col;
        if (doomEnemy[i].col > maxX) maxX = doomEnemy[i].col;
        if (doomEnemy[i].row < minY) minY = doomEnemy[i].row;
        if (doomEnemy[i].row > maxY) maxY = doomEnemy[i].row;
      }
      centerX = (minX + maxX) / 2;
      centerY = (minY + maxY) / 2;
    }
  
    // Render the enemy using a perspective projection.
    // Assumes global variables:
    //   playerX, playerZ  - the player's world position,
    //   projCenterX, projCenterY - the screen center,
    //   focalLength       - the perspective focal length,
    //   doomEnemy[]       - an array of points for the enemy sprite,
    //   numEnemy          - number of pixels in the sprite.
    void render() {
      double dx = worldX - playerX;  // lateral difference from player
      double dz = worldZ - playerZ;  // depth difference from player
  
      // Only render if the enemy is in front of the player.
      if (dz <= 0) return;
  
      // Compute distance from the player and derive a scale factor.
      double distance = std::sqrt(dx * dx + dz * dz);
      if (distance < 1) distance = 1;  // prevent division by zero
      scale = focalLength / distance;
  
      // Compute the enemy's projected anchor position on screen.
      double angleShift = atan2(dx, dz);  // Angle between player and enemy
      double projectedX = projCenterX + tan(angleShift) * focalLength;
      int projectedY = 15; // Fixed vertical position (adjust if you use vertical world coordinates)
  
      // Determine if the enemy is roughly centered.
      enemyCentered = (abs(projectedX - projCenterX) < 10);
  
      // Define a base sprite size. Increase this value to start with a larger image.
      int baseSpriteSize = 10;  // Increased from 10 for a larger starting size
  
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
    bool collidesWithPoint(int x, int z) {
      return pointCollides(worldX, worldZ, x, z, 5, scale);
    }
};


// ########### OBSTACLE LOGIC ##########

class Obstacle {
  public:
    int worldX, worldZ; 
    bool active;
    int centerX, centerY;
    double scale;
    
    // Constructor: world coordinates determine the obstacle's initial placement.
    Obstacle(int wx, int wz)
      : worldX(wx), worldZ(wz), active(true){
        int minX = obstacle[0].col, maxX = obstacle[0].col;
        int minY = obstacle[0].row, maxY = obstacle[0].row;
      for (size_t i = 1; i < numObstacle; i++) {
        if (obstacle[i].col < minX) minX = obstacle[i].col;
        if (obstacle[i].col > maxX) maxX = obstacle[i].col;
        if (obstacle[i].row < minY) minY = obstacle[i].row;
        if (obstacle[i].row > maxY) maxY = obstacle[i].row;
      }
      centerX = (minX + maxX) / 2;
      centerY = (minY + maxY) / 2;
    }
    
    void render() {
      double dx = worldX - playerX;  // lateral difference from player
      double dz = worldZ - playerZ;  // depth difference from player
  
      // Only render if the enemy is in front of the player.
      if (dz <= 0) return;
  
      // Compute distance from the player and derive a scale factor.
      double distance = std::sqrt(dx * dx + dz * dz);
      if (distance < 1) distance = 1;  // prevent division by zero
      scale = focalLength / distance;
  
      // Compute the enemy's projected anchor position on screen.
      double angleShift = atan2(dx, dz);  // Angle between player and enemy
      double projectedX = projCenterX + tan(angleShift) * focalLength;
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
      return pointCollides(worldX, worldZ, x, z, 5, scale);
    }
  };

std::vector<Enemy> enemies;
std::vector<Obstacle> obstacles;

// ######## CHUNK GENERATION LOGIC #########
void generateChunk(int chunkX, int chunkZ) {
  ChunkKey key = {chunkX, chunkZ};

  // Check if this chunk already exists
  if (generatedChunks.find(key) != generatedChunks.end()) {
      return; // Already generated, no need to do anything
  }

  generatedChunks[key] = {chunkX, chunkZ, true};

  // Use a deterministic seed for chunk placement
  long seed = chunkX * 73856093 ^ chunkZ * 19349663;
  randomSeed(seed);

  // Generate enemies and obstacles at fixed positions
  enemies.emplace_back(chunkX * chunkSize + random(-chunkSize / 2, chunkSize / 2),
                       chunkZ * chunkSize + random(-chunkSize / 2, chunkSize / 2));

  obstacles.emplace_back(chunkX * chunkSize + random(-chunkSize / 2, chunkSize / 2),
                        chunkZ * chunkSize + random(-chunkSize / 2, chunkSize / 2));
}

void cleanupOldObjects(int playerChunkX, int playerChunkZ) {
  auto it = generatedChunks.begin();
  while (it != generatedChunks.end()) {
      int chunkX = it->first.x;
      int chunkZ = it->first.z;
      
      // Check if the chunk is out of the render distance
      if (abs(chunkX - playerChunkX) > renderDistanceSide || abs(chunkZ - playerChunkZ) > renderDistanceForward) {
          // Chunk is out of render distance, so remove enemies and obstacles in that chunk
          
          // Clean up enemies
          enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
              [chunkX, chunkZ](const Enemy& e) {
                  return (e.worldX / chunkSize == chunkX && e.worldZ / chunkSize == chunkZ);
              }), enemies.end());
          
          // Clean up obstacles
          obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
              [chunkX, chunkZ](const Obstacle& o) {
                  return (o.worldX / chunkSize == chunkX && o.worldZ / chunkSize == chunkZ);
              }), obstacles.end());
          
          // Erase the chunk from generatedChunks map
          it = generatedChunks.erase(it);  // Remove the chunk from the map
      } else {
          ++it;  // If the chunk is still within the render distance, move to the next chunk
      }
  }
}


void updateWorld() {
  int playerChunkX = playerX / chunkSize;
  int playerChunkZ = playerZ / chunkSize;

  generateChunk(playerChunkX, playerChunkZ);
  generateChunk(playerChunkX+1, playerChunkZ);
  generateChunk(playerChunkX-1, playerChunkZ);
  generateChunk(playerChunkX, playerChunkZ+1);  
  generateChunk(playerChunkX, playerChunkZ+2);  

  // Render all active objects
  for (auto &e : enemies) {
    e.render();
  }
  for (auto &o : obstacles){
    o.render();
  }
  // Cleanup old chunks
  cleanupOldObjects(playerChunkX, playerChunkZ);
}

void checkCollisions() {
  // Check bullet collisions with enemies and obstacles.
  for (int i = 0; i < MAX_BULLETS; i++) {
      if (!bullets[i].active)
          continue;
    
    for (auto &obs : obstacles) {
      if (obs.active && obs.collidesWithPoint(bullets[i].worldX, bullets[i].worldZ)) {
        bullets[i].active = false; // Bullet stops at obstacle.
        break; // Stop checking since bullet is destroyed.
      }
    }

  // If the bullet is still active, check enemy collisions
    if (bullets[i].active) {
      for (auto it = enemies.begin(); it != enemies.end(); ) {
        if (it->active && it->collidesWithPoint(bullets[i].worldX, bullets[i].worldZ)) {
          score += 100;              // Increase score.
          bullets[i].active = false; // Deactivate bullet.
          it = enemies.erase(it);    // Remove the enemy and get the next iterator.
          break; // Bullet disappears after hitting one enemy.
        } else {
          ++it;
        }
      }
    }

  }
  
  // Check player collision with enemies: if player touches an enemy, game over.
  for (auto &enemy : enemies) {
      if (enemy.active && enemy.collidesWithPoint(playerX, playerZ)) {
          u8g2.clearBuffer();
          for (int i = 0; i < numGameOver; i++) {
            u8g2.drawPixel(gameOver[i].col, gameOver[i].row);
          }
          u8g2.sendBuffer();
          xSemaphoreTake(sysState.mutex, portMAX_DELAY);
          sysState.activityList = MENU;
          xSemaphoreGive(sysState.mutex);
          vTaskDelay(1000 / portTICK_PERIOD_MS);
          showGun=false;
        }
  }
}

void updatePlayerPosition(int jx, int jy) {

  double newPlayerX = playerX;
  double newPlayerZ = playerZ;
  
  if (jx > 600 || jx < 400) {
      newPlayerX += (jx - 460) * 0.005;
  }
  if (jy < 400 || jy > 600) {
      newPlayerZ += (516 - jy) * 0.005;
  }
  
  // Check for collision with any obstacle.
  bool collision = false;
  for (auto &obs : obstacles) {
      if (obs.active && obs.collidesWithPoint(newPlayerX, newPlayerZ)) {
          collision = true;
          playerX = newPlayerX;
          break;
      }
  }
  
  // Only update the player's position if there is no collision.
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
    u8g2.setDrawColor(1);  // 1 = white, 0 = black
  
      // Render the loading image (from doomLoadScreen)
    for (size_t i = 0; i < numLoadOnes; i++) {
      u8g2.drawPixel(doomLoadScreen[i].col, doomLoadScreen[i].row);
    }
    u8g2.sendBuffer();
    enemies.clear();
    enemies.shrink_to_fit();
    obstacles.clear();
    obstacles.shrink_to_fit();
    enemies.emplace_back(30, 20);
    obstacles.emplace_back(-30, 30);
    playerX = 0;
    playerZ = -20;
    showGun=true;
    score=0;
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

  updatePlayerPosition(jx, jy);
  updateWorld();
  
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
  }
  u8g2.sendBuffer();
}