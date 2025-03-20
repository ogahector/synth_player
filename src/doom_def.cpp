//Doom source file
#include <globals.h>
#include <doom_def.h>
#include <vector>
#include <array>

const int projCenterX = 64;
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

uint8_t note[8];

// ###### CHUNK LOGIC ######
int chunkSize = 100;
const int renderDistanceSide = 1;
const int renderDistanceForward = 2; 

#define MAX_CHUNKS (2*renderDistanceSide +1) * (renderDistanceForward +1)

// ####### BULLET LOGIC ########
#define MAX_BULLETS 1 

struct Bullet {
    float worldX, worldZ, worldY; 
    float vX, vZ, vY;        
    bool active;           
};

Bullet bullets[MAX_BULLETS];  

// Initialise bullets: mark all as inactive.
void initBullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }
}

// Shoot a bullet from the gun.
void shootBullet() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            
            bullets[i].worldX = playerX;      
            bullets[i].worldZ = playerZ + 5;   
            bullets[i].worldY = 19; 
          
            bullets[i].vX = -1;
            bullets[i].vZ = 3; 
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
      bullets[i].worldX += bullets[i].vX;
      bullets[i].worldZ += bullets[i].vZ;
      bullets[i].worldY += bullets[i].vY;
      if (bullets[i].worldZ > playerZ + 50) {
          bullets[i].active = false;
          continue;
      }

      float dx = bullets[i].worldX - playerX;
      float dz = bullets[i].worldZ - playerZ;
      float dy = bullets[i].worldY - 19;
      if (dz <= 0) {
        bullets[i].active = false;
        continue;
      }
      float distance = std::sqrt(dx * dx + dz * dz);
      if (distance < 1) distance = 1;  
      float scale = focalLength / distance;

      int screenX = 69 + static_cast<int>(dx * scale);
      int screenY = 19 + static_cast<int>(dy * scale);;
      
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
    int worldX, worldZ;
    bool active;
    int centerX=60;
    int centerY=16;
    float scale;

    Enemy() : worldX(0), worldZ(0), active(false) {}
    Enemy(int wx, int wz)
      : worldX(wx), worldZ(wz), active(true) {}
  
    void render() {
      float dx = worldX - playerX;  
      float dz = worldZ - playerZ;  
  
      if (dz <= 0) return;
  
      float distance = std::sqrt(dx * dx + dz * dz);
      if (distance < 1) distance = 1; 
      scale = focalLength / distance;
  
      float angleShift = atan2(dx, dz);  
      float projectedX = projCenterX + tan(angleShift) * focalLength;
      int projectedY = 15;
      int baseSpriteSize = 5;  
  
      for (size_t i = 0; i < numEnemy; i++) {
        int spriteX = doomEnemy[i].col;
        int spriteY = doomEnemy[i].row;
        int localOffsetX = spriteX - centerX;
        int localOffsetY = spriteY - centerY;
        int renderX = projectedX + static_cast<int>(localOffsetX * scale);
        int renderY = projectedY + static_cast<int>(localOffsetY * scale);
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
    Obstacle(int wx, int wz)
      : worldX(wx), worldZ(wz), active(true){}
    
    void render() {
      float dx = worldX - playerX;  
      float dz = worldZ - playerZ;  
  
      if (dz <= 0) return;
      float distance = std::sqrt(dx * dx + dz * dz);
      if (distance < 1) distance = 1; 
      scale = focalLength / distance;
  
      float angleShift = atan2(dx, dz);  
      float projectedX = projCenterX + tan(angleShift) * focalLength;
      int projectedY = 15; 
  
      int baseSpriteSize = 20;  
  
      for (size_t i = 0; i < numObstacle; i++) {
        int spriteX = obstacle[i].col;
        int spriteY = obstacle[i].row;
        int localOffsetX = spriteX - centerX;
        int localOffsetY = spriteY - centerY;
        int renderX = projectedX + static_cast<int>(localOffsetX * scale);
        int renderY = projectedY + static_cast<int>(localOffsetY * scale);
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
  for (int x = -renderDistanceSide; x <= renderDistanceSide; x++) {
    for (int z = 0; z <= renderDistanceForward; z++) {
      generateChunk(playerChunkX + x, playerChunkZ + z);
    }
  }
  for (int i = 0; i < MAX_CHUNKS; i++) {
    if (!chunkStorage[i].isGenerated) continue;
    if (chunkStorage[i].enemyActive) {
      chunkStorage[i].enemy.render();
    }
    if (chunkStorage[i].obstacleActive) {
      chunkStorage[i].obstacle.render();
    }
  }
  cleanupOldObjects(playerChunkX, playerChunkZ);
}

void checkCollisions() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active)
      continue;
    for (int c = 0; c < MAX_CHUNKS; c++) {
      if (!chunkStorage[c].isGenerated) continue;
      if (chunkStorage[c].obstacleActive && chunkStorage[c].obstacle.collidesWithPoint(bullets[i].worldX, bullets[i].worldZ)) {
        bullets[i].active = false;
        break;
        }
      if (!bullets[i].active) break;

      if (chunkStorage[c].enemyActive && chunkStorage[c].enemy.collidesWithPoint(bullets[i].worldX, bullets[i].worldZ, false)) {
        uint8_t octave = rand() % 8 + 1;
        uint8_t key = rand() % 12 + 1;
        note[0]='P'; note[1]=octave; note[2]=key; note[3]= 8; note[4]=0; note[5]=0; note[6]=0; note[7]=0;
        xSemaphoreTake(sysState.mutex,portMAX_DELAY);
        if (sysState.slave) xQueueSend(msgOutQ, note, portMAX_DELAY);
        else xQueueSend(msgInQ,note,portMAX_DELAY);
        xSemaphoreGive(sysState.mutex);
        score += 100;                      
        bullets[i].active = false;         
        chunkStorage[c].enemyActive = false;
        killedMonster=true;
        break;
      }
    }
    if (!bullets[i].active) break; 
  }

  // Enemy-player collisions
  for (int c = 0; c < MAX_CHUNKS; c++) {
    if (!chunkStorage[c].isGenerated) continue; 
    if (chunkStorage[c].enemyActive && chunkStorage[c].enemy.collidesWithPoint(playerX, playerZ, true)) {
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
      return;  
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
  if (!doomLoadingShown) {
    u8g2.clearBuffer();
    u8g2.setDrawColor(1);
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

  updateBullets();

  updatePlayerPosition(jx, jy);
  
  u8g2.setCursor(100,10);
  u8g2.print(score, DEC);
  
  checkCollisions();

  if(showGun){
  // Draw the gun
    u8g2.setDrawColor(0);
    for (size_t i = 0; i < numGunOutline; i++) {
      u8g2.drawPixel(gunOutline[i].col + 10, gunOutline[i].row); 
    }
    u8g2.setDrawColor(1);
    for (size_t i = 0; i < numGun; i++) {
      u8g2.drawPixel(doomGun[i].col + 10, doomGun[i].row);
    }
    frameCount++;
    unsigned long currentTime = micros();
    if (currentTime - lastFpsUpdate >= 1000) {
      fps = frameCount / ((currentTime - lastFpsUpdate) / 1000000.0);
      frameCount = 0;
      lastFpsUpdate = currentTime;
    }
    u8g2.setCursor(0, 10);
    u8g2.print("FPS: ");
    u8g2.print(fps, 1);
  }
  
  if (killedMonster){
    count++;
    if (count==20){
      count=0;
      killedMonster=false;
      note[0]='R';
      xSemaphoreTake(sysState.mutex,portMAX_DELAY);
      if (sysState.slave) xQueueSend(msgOutQ, note, portMAX_DELAY);
      else xQueueSend(msgInQ,note,portMAX_DELAY);
      xSemaphoreGive(sysState.mutex);
    }
  }
  u8g2.sendBuffer();
}