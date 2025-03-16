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


// ###### CHUNK LOGIC ######

int chunkSize = 128;
const int renderDistanceSide = 1; // Side render distance
const int renderDistanceForward = 3; // More forward chunks
const int renderDistanceBackward = 1; // Fewer backward chunks

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

// ########### ENEMY LOGIC ##########
class Enemy {
  public:
    int worldX, worldZ; // Enemy's world coordinates (x = lateral, z = depth)
    bool active;
    bool enemyCentered;
    
    // Constructor: world coordinates determine the enemy's initial placement.
    Enemy(int wx, int wz)
      : worldX(wx), worldZ(wz), active(true), enemyCentered(false) {}
  
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
      double scale = focalLength / distance;
  
      // Compute the enemy's projected anchor position on screen.
      double angleShift = atan2(dx, dz);  // Angle between player and enemy
      double projectedX = projCenterX + tan(angleShift) * focalLength;
      int projectedY = 10; // Fixed vertical position (adjust if you use vertical world coordinates)
  
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
        int localOffsetX = spriteX - doomEnemy[0].col;
        int localOffsetY = spriteY - doomEnemy[0].row;
        // Scale the local offsets.
        int renderX = projectedX + static_cast<int>(localOffsetX * scale);
        int renderY = projectedY + static_cast<int>(localOffsetY * scale);
        // Draw the pixel.
        u8g2.drawPixel(renderX, renderY);
      }
    }

        // bool checkBulletCollision(const Bullet &b) {
        //   if (!active) return false;

        //   if (b.x >= worldX - scaledSize / 2 && b.x <= worldX + scaledSize / 2 &&
        //     b.y >= worldZ - scaledSize / 2 && b.y <= worldZ + scaledSize / 2) {
        //     return true;
        // }
        //   int size = 3;
        //   // Simple bounding box collision detection
        //   if (b.x >= worldX && b.x <= worldX + size &&
        //       b.y >= worldZ && b.y <= worldZ + size) {
        //       return true;
        //   }
        //   return false;
       // }

        // bool checkPlayerCollision() {
        //   if (!active) return false;
        //   if (enemyCentered && scale>2.4){
        //     return true;
        //   }
        //   return false;
        // }
};


// ########### OBSTACLE LOGIC ##########

class Obstacle {
  public:
    int worldX, worldZ; 
    bool active;
    bool obstacleCentered;
    
    // Constructor: world coordinates determine the obstacle's initial placement.
    Obstacle(int wx, int wz)
      : worldX(wx), worldZ(wz), active(true){}
  
    
    void render() {
      double dx = worldX - playerX;  // lateral difference from player
      double dz = worldZ - playerZ;  // depth difference from player
  
      // Only render if the enemy is in front of the player.
      if (dz <= 0) return;
  
      // Compute distance from the player and derive a scale factor.
      double distance = std::sqrt(dx * dx + dz * dz);
      if (distance < 1) distance = 1;  // prevent division by zero
      double scale = focalLength / distance;
  
      // Compute the enemy's projected anchor position on screen.
      double angleShift = atan2(dx, dz);  // Angle between player and enemy
      double projectedX = projCenterX + tan(angleShift) * focalLength;
      int projectedY = 0; // Fixed vertical position (adjust if you use vertical world coordinates)
  
      // Determine if the enemy is roughly centered.
      obstacleCentered = (abs(projectedX - projCenterX) < 10);
  
      // Define a base sprite size. Increase this value to start with a larger image.
      int baseSpriteSize = 20;  // Increased from 10 for a larger starting size
  
      // Now render the enemy sprite. We assume doomEnemy[0] is the anchor.
      for (size_t i = 0; i < numObstacle; i++) {
        // Get the local coordinates of this pixel in the sprite.
        int spriteX = obstacle[i].col;
        int spriteY = obstacle[i].row;
        // Calculate the offset from the sprite's anchor (doomEnemy[0]).
        int localOffsetX = spriteX - obstacle[0].col;
        int localOffsetY = spriteY - obstacle[0].row;
        // Scale the local offsets.
        int renderX = projectedX + static_cast<int>(localOffsetX * scale);
        int renderY = projectedY + static_cast<int>(localOffsetY * scale);
        // Draw the pixel.
        u8g2.drawPixel(renderX, renderY);
      }
    }
  
      bool checkBulletCollision(const Bullet &b) {
        
        // int size = 3;
        // // Simple bounding box collision detection
        // if (b.x >= posX && b.x <= posX + size &&
        //     b.y >= posY && b.y <= posY + size) {
        //     return true;
        // }
        // return false;
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

  Serial.print("Generating chunk: ");
  Serial.print(chunkX); Serial.print(", "); Serial.println(chunkZ);

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
  if (jy<400 || jy>600){
    playerZ += (516-jy) * 0.005;
  }
  if (jx>600 || jx<400){
    playerX += (jx - 460) * 0.005;
  }

    
  updateWorld();
  
    // if (it->checkPlayerCollision()){
    //   u8g2.clearBuffer();
    //   for (int i = 0; i < numGameOver; i++) {
    //     u8g2.drawPixel(gameOver[i].col, gameOver[i].row);
    //   }
    //   u8g2.sendBuffer();
    //   vTaskDelay(1000 / portTICK_PERIOD_MS);
    //   xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    //   sysState.activityList = MENU;
    //   xSemaphoreGive(sysState.mutex);
    //   showGun=false;
    // }


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