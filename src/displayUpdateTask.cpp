//Display update source
#include <globals.h>
#include <U8g2lib.h>
#include <displayUpdateTask.h>
#include <menu.h>
#include <doom_def.h>
#include <waves.h>
#include <home.h>

bool doomLoadingShown=false;

void displayUpdateTask(void* vParam)
{
  const TickType_t xFrequency = 50/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  int localActivity = -1;
  int localJoystickDir = 0;
  static int previousActivity = -1;
  
  while(1)
  {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
    // Serial.println("Display");
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);

    switch (sysState.activityList)
    {
      case MENU:
        localActivity = 1;
        break;
      case DOOM:
        localActivity = 2;
        break;
      case WAVE:
        localActivity = 3;
        break;
      case HOME:
        localActivity = 0;
        break;
      default:
        localActivity = -1;
        break;
    }
    localJoystickDir = sysState.joystickHorizontalDirection;
    xSemaphoreGive(sysState.mutex);  


    if (localActivity == 2)
    {
      // Transitioning into doom state: if we weren't in doom previously, trigger loading screen.
      if (previousActivity != 2) {
         doomLoadingShown = false; // Show loading screen
      }
      else {
         doomLoadingShown = true;  // Already in doom; no need to show the loading screen again.
      }
    }
    else {
      // For non-doom activities, reset the flag so that if we return to doom, the loading screen appears.
      doomLoadingShown = true;
    }
    // Now, outside the critical section, do the rendering.
    switch (localActivity)
    {
      case 1:
        renderMenu();
        break;
      case 2:
        renderDoomScene(doomLoadingShown);
        break;
      case 3: {
        int selection = renderWaves();
        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        sysState.currentWaveform = static_cast<waveform_t>(selection);
        xSemaphoreGive(sysState.mutex);
        break;
      }
      case 0:
        renderHome();
        break;
      default:
        u8g2.clearBuffer();
        u8g2.setCursor(0, 10);
        u8g2.print("There was an error");
        u8g2.sendBuffer();
        break;
    }
    previousActivity = localActivity;
  }
}