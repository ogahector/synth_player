//Doom source file
#include <globals.h>
#include <menu.h>
#include <menu_icons.h>

int SCREEN_WIDTH=u8g2.getDisplayWidth();
int SCREEN_HEIGHT=u8g2.getDisplayHeight();

#define ICON_WIDTH  90
#define ICON_HEIGHT 28

int currentMenuIndex = 2;

void drawIcon(const Pixel *icon, size_t numPixels, int offsetX, int offsetY, int scaleFactor) {
    for (size_t i = 0; i < numPixels; i++) {
        int x = offsetX + icon[i].col * scaleFactor;
        int y = offsetY + icon[i].row * scaleFactor;
        u8g2.drawPixel(x, y);
    }
}


void animateMenuTransition(int currentIndex, int direction) {
    int steps = 10;  // Increase for smoother transition
    int offsetTotal = SCREEN_WIDTH; // Total horizontal move distance

    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;

    int newIndex = currentIndex + direction;
    if (newIndex < 2) {
        newIndex = 3;
    } else if (newIndex > 3) {
        newIndex = 2;
    }

    // For each animation frame, compute an easing value.
    for (int s = 0; s <= steps; s++) {
        float t = (float)s / steps;
        // Ease in/out function (cosine-based)
        float ease = 0.5 * (1 - cos(M_PI * t));

        // Calculate x positions for the current icon and the new icon.
        int currentIconX = centerX - ICON_WIDTH / 2 - (int)(ease * offsetTotal) * direction - 15;
        int newIconX = centerX - ICON_WIDTH / 2 + (offsetTotal - (int)(ease * offsetTotal)) * direction - 15;
        int iconY = centerY - ICON_HEIGHT / 2 - 2;  // Center the icon vertically

        // Clear the screen for each frame
        u8g2.clearBuffer();

        // Draw current icon if the transition isn't complete.
        if (s < steps) {
            switch (currentMenuIndex) {
                case 2:
                    drawIcon(doomIcon, numDoomIcon, currentIconX, iconY, 1);
                    break;
                case 3:
                    drawIcon(waveIcon, numWaveIcon, currentIconX, iconY, 1);
                    break;
                default:
                    u8g2.setCursor(10, 10);
                    u8g2.print("Error");
                    break;
            }
        }

        // Draw the new icon
        switch (newIndex) {
            case 2:
                drawIcon(doomIcon, numDoomIcon, newIconX, iconY, 1);
                break;
            case 3:
                drawIcon(waveIcon, numWaveIcon, newIconX, iconY, 1);
                break;
            default:
                u8g2.setCursor(10, 10);
                u8g2.print("Error");
                break;
        }

        // Send buffer to display
        u8g2.sendBuffer();
        delay(20);  // Adjust delay for smoother or faster animation
    }

    // Update the current menu index.
    currentMenuIndex = newIndex;
}


void renderMenu(){
    u8g2.clearBuffer();
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    int direction = sysState.joystickHorizontalDirection;
    xSemaphoreGive(sysState.mutex);
    if (direction > 650){
        direction = 1;
    }
    else if (direction < 300){
        direction = -1;
    }
    else {
        direction = 0;
    }
    u8g2.setCursor(10, 10);
    u8g2.print(direction);
    animateMenuTransition(currentMenuIndex, direction);
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    if (sysState.joystickPress) {
        sysState.activityList[currentMenuIndex] = true;
        sysState.activityList[1] = false;
        sysState.joystickPress = false;
    }
    xSemaphoreGive(sysState.mutex);
    u8g2.sendBuffer();
}