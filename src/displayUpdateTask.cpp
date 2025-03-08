//Display update source
#include <globals.h>
#include <U8g2lib.h>
#include <displayUpdateTask.h>
#include <menu.h>
#include <doom_def.h>
#include <waves.h>
#include <home.h>


void displayUpdateTask(void* vParam)
{
  const TickType_t xFrequency = 50/portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  int localActivity = -1;
  int localJoystickDir = 0;

  while(1)
  {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    xSemaphoreTake(sysState.mutex, portMAX_DELAY);

    if (sysState.activityList.test(1))
      localActivity = 1;
    else if (sysState.activityList.test(2))
      localActivity = 2;
    else if (sysState.activityList.test(3))
      localActivity = 3;
    else if (sysState.activityList.test(0))
      localActivity = 0;
    else
      localActivity = -1;
    
    localJoystickDir = sysState.joystickDirection;
    xSemaphoreGive(sysState.mutex);
    // Now, outside the critical section, do the rendering.
    switch (localActivity)
    {
      case 1:
        renderMenu();
        break;
      case 2:
        renderDoomScene();
        break;
      case 3:
        renderWaves();
        break;
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
  }
}