//Display update source
#include <globals.h>
#include <U8g2lib.h>
#include <displayUpdateTask.h>
#include <menu.h>
#include <doom_def.h>
#include <waves.h>
#include <home.h>
#include <recording.h>

bool doomLoadingShown=false;
bool alreadyShown=false;

void displayUpdateTask(void* vParam)
{
  const TickType_t xFrequency = 50/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  activityList_t localActivity = NONE;
  int localJoystickDir = 0;
  static activityList_t previousActivity = NONE;
  
  while(1)
  {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    #ifdef GET_MASS_DISPLAY
    monitorStackSize();
    #endif

    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    localActivity = sysState.activityList;
    localJoystickDir = sysState.joystickHorizontalDirection;
    xSemaphoreGive(sysState.mutex);  


    if (localActivity == DOOM)
    {
      // Transitioning into doom state: if we weren't in doom previously, trigger loading screen.
      if (previousActivity != DOOM) {
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
    if (localActivity == RECORDING)
    {
      // Transitioning into doom state: if we weren't in doom previously, trigger loading screen.
      if (previousActivity != RECORDING) {
        alreadyShown = false; // Show loading screen
      }
      else {
        alreadyShown = true;  // Already in doom; no need to show the loading screen again.
      }
    }
    else {
      // For non-doom activities, reset the flag so that if we return to doom, the loading screen appears.
      alreadyShown = true;
    }
    // Now, outside the critical section, do the rendering.
    switch (localActivity)
    {
      case MENU:
        renderMenu();
        break;
      case DOOM:
        renderDoomScene(doomLoadingShown);
        break;
      case WAVE: {
        int selection = renderWaves();
        xSemaphoreTake(sysState.mutex, portMAX_DELAY);
        sysState.currentWaveform = static_cast<waveform_t>(selection);
        xSemaphoreGive(sysState.mutex);
        break;
      }
      case RECORDING:
        renderRecording(alreadyShown);
        break;
      case HOME:
        renderHome();
        break;
      case NONE:
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