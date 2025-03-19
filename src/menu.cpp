//Doom source file
#include <globals.h>
#include <menu.h>
#include <menu_icons.h>

int SCREEN_WIDTH=u8g2.getDisplayWidth();
int SCREEN_HEIGHT=u8g2.getDisplayHeight();

#define ICON_WIDTH  90
#define ICON_HEIGHT 28

int currentMenuIndex = 2;

void drawRecIcon(int offsetX, int offsetY) {
    u8g2.drawRFrame(35+offsetX, 5+offsetY, 59, 16, 3);
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(57+offsetX, 17+offsetY, "REC");
    u8g2.setFont(u8g2_font_ncenB08_tr); // start font
    u8g2.drawDisc(50+offsetX, 13+offsetY, 3, U8G2_DRAW_ALL);
    u8g2.drawStr(35+offsetX, 30+offsetY, "Recording");
}

void drawWaveIcon(const Pixel *icon, size_t numPixels, int offsetX, int offsetY) {
    u8g2.drawRFrame(35+offsetX, 5+offsetY, 59, 16, 3);
    for (size_t i = 0; i < numPixels; i++) {
        int x = offsetX + icon[i].col;
        int y = offsetY + icon[i].row;
        u8g2.drawPixel(x, y);
    }
    u8g2.drawStr(32+offsetX, 30+offsetY, "Wave Select");
}

void drawDoomIcon(const Pixel *icon, size_t numPixels, int offsetX, int offsetY) {
    u8g2.drawRFrame(35+offsetX, 5+offsetY, 59, 16, 3);
    for (size_t i = 0; i < numPixels; i++) {
        int x = offsetX + icon[i].col * 0.6 + 26;
        int y = offsetY + icon[i].row * 0.6 + 4;
        u8g2.drawPixel(x, y);
    }
    u8g2.drawStr(48+offsetX, 30+offsetY, "Doom");
}


void animateMenuTransition(int currentIndex, int direction) {
    int steps = 8;  // Increase for smoother transition
    int offsetTotal = SCREEN_WIDTH; // Total horizontal move distance

    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;

    int newIndex = currentIndex + direction;
    if (newIndex < 2) {
        newIndex = 4;
    } else if (newIndex > 4) {
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
                    drawDoomIcon(doomLoadScreen, numLoadOnes, currentIconX, iconY);
                    break;
                case 3:
                    drawWaveIcon(waveIcon, numWaveIcon, currentIconX, iconY);
                    break;
                case 4:
                    drawRecIcon(currentIconX, iconY);
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
                drawDoomIcon(doomLoadScreen, numLoadOnes, newIconX, iconY);
                break;
            case 3:
                drawWaveIcon(waveIcon, numWaveIcon, newIconX, iconY);
                break;
            case 4:
                drawRecIcon(newIconX, iconY);
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
        sysState.joystickPress = false;
        switch (currentMenuIndex)
        {
            case 2:
                sysState.activityList = DOOM;
                break;
            case 3: 
                sysState.activityList = WAVE;
                break;
            case 4:
                sysState.activityList = RECORDING;
                break;
            default:
                break;
        }
    }
    xSemaphoreGive(sysState.mutex);
    u8g2.sendBuffer();
}