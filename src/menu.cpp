//Doom source file
#include <globals.h>
#include <menu.h>
#include <menu_icons.h>

int SCREEN_WIDTH=u8g2.getDisplayWidth();
int SCREEN_HEIGHT=u8g2.getDisplayHeight();

#define ICON_WIDTH  80
#define ICON_HEIGHT 25

int currentMenuIndex = 2;

void drawIcon(const Pixel *icon, size_t numPixels, int offsetX, int offsetY, int scaleFactor) {
    for (size_t i = 0; i < numPixels; i++) {
        int x = offsetX + icon[i].col * scaleFactor;
        int y = offsetY + icon[i].row * scaleFactor;
        u8g2.drawPixel(x, y);
    }
}


void animateMenuTransition(int currentIndex, int direction) {
    // direction: 1 for swiping left (new icon comes from right), -1 for swiping right.
    int steps = 10;  // Increase for a smoother transition.
    int offsetTotal = SCREEN_WIDTH; // How far the icon moves horizontally.
    
    int centerX = (SCREEN_WIDTH  - ICON_WIDTH) / 2;
    int centerY = (SCREEN_HEIGHT - ICON_HEIGHT) / 2;
    int newIndex = currentIndex + direction;
    // For each animation frame, compute an easing value.
    for (int s = 0; s <= steps; s++) {
        float t = (float)s / steps;
        // Ease in/out function (cosine-based)
        float ease = 0.5 * (1 - cos(M_PI * t));
        
        // Calculate x positions for the current icon and the new icon.
        // The current icon is moving out, the new icon is moving in.
        int currentIconX = (SCREEN_WIDTH / 2) - (ICON_WIDTH / 2) - (int)(ease * offsetTotal) * direction;
        int newIconX = (SCREEN_WIDTH / 2) - (ICON_WIDTH / 2) + (offsetTotal - (int)(ease * offsetTotal)) * direction;
        
        
        // The y position remains centered.
        int iconY = (SCREEN_HEIGHT - ICON_HEIGHT) / 2;
                
        // Draw current icon if the transition isn't complete.
        if (s < steps) {
            switch (currentMenuIndex){
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
        switch (newIndex){
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
    // Update the current menu index.
    currentMenuIndex = newIndex;
}

void renderMenu(){
    u8g2.clearBuffer();
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    int direction = sysState.joystickDirection;
    xSemaphoreGive(sysState.mutex);
    u8g2.setCursor(10, 10);
    u8g2.print(direction);
    direction =0;

    animateMenuTransition(currentMenuIndex, direction);
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    if (sysState.joystickPress) {
        sysState.activityList[currentMenuIndex] = true;
    }
    xSemaphoreGive(sysState.mutex);
    u8g2.sendBuffer();
}