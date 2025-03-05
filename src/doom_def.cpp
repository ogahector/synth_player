//Doom source file
#include <globals.h>
#include <doom.h>
#include <doom_def.h>

// Show DOOM loading screen
bool doomLoadingShown = false;


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
      u8g2.drawPixel(doomGun[i].col, doomGun[i].row);
    }
    // for (size_t i = 0; i < numEnemy; i++) {
    //   u8g2.drawPixel(doomEnemy[i].col-30, doomEnemy[i].row);
    // }
    u8g2.sendBuffer();
  }