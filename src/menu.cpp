//Doom source file
#include <globals.h>
#include <menu.h>
#include <menu_icons.h>
#include <gpio.h>

int SCREEN_WIDTH=u8g2.getDisplayWidth();
int SCREEN_HEIGHT=u8g2.getDisplayHeight();

#define ICON_WIDTH  90
#define ICON_HEIGHT 28

int currentMenuIndex = 2;

void drawRecIcon(int offsetX, int offsetY) {
    u8g2.drawRFrame(35+offsetX, 5+offsetY, 59, 16, 3);
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.drawStr(57+offsetX, 17+offsetY, "REC");
    u8g2.setFont(u8g2_font_ncenB08_tr);
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

// Precompute at compile time
#define STEPS 10
const int iconY = (SCREEN_HEIGHT - ICON_HEIGHT) / 2 - 2;

struct Offsets {
    int currentOffsets[STEPS];
    int newOffsets[STEPS];
};

const Offsets preAllocateSteps(int direction)
{
    Offsets off;
    for (size_t i = 0; i < STEPS; i++) {
        float t = (float)i / STEPS;
        float ease = 0.5 * (1 - cos(M_PI * t));
        off.currentOffsets[i] = (SCREEN_WIDTH - ICON_WIDTH) / 2 - (int)(ease * SCREEN_WIDTH) * direction - 15;
        off.newOffsets[i]     = (SCREEN_WIDTH - ICON_WIDTH) / 2 + (SCREEN_WIDTH - (int)(ease * SCREEN_WIDTH)) * direction - 15;
    }
    return off;
}

const Offsets stepsPos = preAllocateSteps(1);

const Offsets stepsNeg = preAllocateSteps(-1);

void animateMenuTransition(int currentIndex, int direction, bool slave) {
    int newIndex = currentIndex + direction;
    if (newIndex < 2) {
        newIndex = 4;
    } else if (newIndex > 4) {
        newIndex = 2;
    }
    int currentIconX;
    int newIconX;

    // For each frame, update the positions based on the precomputed arrays.
    for (int s = 0; s < STEPS; s++) {
        if (direction == 1) {
            currentIconX = stepsPos.currentOffsets[s];
            newIconX     = stepsPos.newOffsets[s];
        } else if (direction == -1) {
            currentIconX = stepsNeg.currentOffsets[s];
            newIconX     = stepsNeg.newOffsets[s];
        } else {
            currentIconX = (SCREEN_WIDTH - ICON_WIDTH) / 2 - 15;
            newIconX     = (SCREEN_WIDTH - ICON_WIDTH) / 2 - 15;
        }
        
        u8g2.clearBuffer();
        
        // Draw the old (current) icon that is sliding out.
        switch (currentIndex) {
            case 2:
                drawDoomIcon(doomLoadScreen, numLoadOnes, currentIconX, iconY);
                break;
            case 3:
                if (!slave){
                    drawWaveIcon(waveIcon, numWaveIcon, currentIconX, iconY);
                }
                else{
                    newIndex=2;
                }
                break;
            case 4:
                if (!slave){
                    drawRecIcon(currentIconX, iconY);
                }
                else{
                    newIndex=2;
                }
                break;
            default:
                break;
        }
        
        // Draw the new icon that is sliding in.
        switch (newIndex) {
            case 2:
                drawDoomIcon(doomLoadScreen, numLoadOnes, newIconX, iconY);
                break;
            case 3:
                if(!slave){
                    drawWaveIcon(waveIcon, numWaveIcon, newIconX, iconY);
                }
                else{
                    newIndex=2;
                }
                break;
            case 4:
                if(!slave){
                    drawRecIcon(newIconX, iconY);
                }
                else{
                    newIndex=2;
                }
                break;
            default:
                break;
        }
        u8g2.sendBuffer();
    }
    currentMenuIndex = newIndex;
}
void renderMenu(){
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    int direction = sysState.joystickHorizontalDirection;
    bool slave = sysState.slave;
    xSemaphoreGive(sysState.mutex);
    if (direction > JOYX_THRESHOLD_LEFT_MENU){
        direction = 1;
    }
    else if (direction < JOYX_THRESHOLD_RIGHT_MENU){
        direction = -1;
    }
    else {
        direction = 0;
    }
    animateMenuTransition(currentMenuIndex, direction, slave);
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
}