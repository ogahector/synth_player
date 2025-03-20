#include <STM32FreeRTOS.h>
#include <knob.h>
#include <globals.h>

// Indices for waveforms:
// 0 = sine, 1 = square, 2 = sawtooth, 3 = triangle. 
const int boxWidth = u8g2.getDisplayWidth() / 2 - 2;
const int boxHeight = u8g2.getDisplayHeight() / 2 - 2;
const int margin = 2; // margin within each box for drawing the waveform
const int spacing = 1;

static int selection=0;

int renderWaves() {
    // Clear display buffer
    u8g2.clearBuffer();
  
    // Draw each of the four waveform boxes
    for (int i = 0; i < 4; i++) {
        int col = i % 2;
        int row = i / 2;
        int x = col * (boxWidth + spacing) + spacing;
        int y = row * (boxHeight + spacing) + spacing;

  
        // If this box is selected, highlight it by filling the box.
        if (i == selection) {
          u8g2.drawRBox(x, y, boxWidth, boxHeight,3);
          // Then, set draw color to 0 so that the waveform is drawn in the inverted color.
          u8g2.setDrawColor(0);
        } else {
          // Draw a simple frame for non-selected boxes.
          u8g2.drawRFrame(x, y, boxWidth, boxHeight,3);
          u8g2.setDrawColor(1);
        }
  
        // Define the drawing area for the waveform inside the box.
        int wx = x + margin;
        int wy = y + margin;
        int wWidth = boxWidth - 2 * margin;
        int wHeight = boxHeight - 2 * margin;
        float period = 2;
        // Draw the appropriate waveform.
        switch (i) {
            case 0: { // Sine wave
                for (int px = 0; px < wWidth; px++) {
                    float t = (float)px / (wWidth - 1) * 2 * PI * period;
                    float val = sin(t);
                    // Scale sine value to fit the drawing area.
                    int py = wy + (wHeight / 2) - round(val * (wHeight / 2));
                    u8g2.drawPixel(wx + px, py);
                }
                break;
            }
            case 1:  {// Square wave
                int prevY = -1; // initialize to an impossible value
                for (int px = 0; px < wWidth; px++) {
                    float t = (float)px / (wWidth - 1) * (2 * PI * period);
                    int currentY = (sin(t) >= 0) ? (wy + margin) : (wy + wHeight - margin);
              
                // If not the first pixel and there's a jump, fill in the vertical gap.
                    if (px > 0 && currentY != prevY) {
                        int yStart = (prevY < currentY) ? prevY : currentY;
                        int yEnd   = (prevY > currentY) ? prevY : currentY;
                        for (int y = yStart; y <= yEnd; y++) {
                             u8g2.drawPixel(wx + px, y);
                        }
                    }
              
                // Draw the current pixel.
                 u8g2.drawPixel(wx + px, currentY);
                prevY = currentY;
                }
                break;
            }
            case 2: { // Sawtooth wave
                int prevYsaw = -1;
                for (int px = 0; px < wWidth; px++) {
                    // Calculate t over multiple cycles and extract fractional part.
                    float t = fmod((float)px / (wWidth - 1) * period, 1.0);
                    int currentY = wy + wHeight - round(t * wHeight);
            
                    if (px > 0 && currentY != prevYsaw) {
                        int yStart = (prevYsaw < currentY) ? prevYsaw : currentY;
                        int yEnd   = (prevYsaw > currentY) ? prevYsaw : currentY;
                        for (int y = yStart; y <= yEnd; y++) {
                            u8g2.drawPixel(wx + px, y);
                        }
                    }
            
                    u8g2.drawPixel(wx + px, currentY);
                    prevYsaw = currentY;
                }
                break;
            }
            case 3: { // Triangle wave
                for (int px = 0; px < wWidth; px++) {
                    float t = fmod((float)px / (wWidth - 1) * period, 1.0);
                    int py;
                    if (t < 0.5) {
                        py = wy + wHeight - round((t * 2) * wHeight);
                    }
                    else {
                        py = wy + round((t - 0.5) * 2 * wHeight);
                    }
                    u8g2.drawPixel(wx + px, py);
                }
                break;
            }
        }
        // Reset drawing color to white for the next box.
        u8g2.setDrawColor(1);
    }
  
      // Send the buffer to update the display.
    u8g2.sendBuffer();
      
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
      // Read joystick data (replace with your actual method to get joystick values)
    int jx = sysState.joystickHorizontalDirection;          // e.g., -1 to 1
    int jy = sysState.joystickVerticalDirection;          // e.g., -1 to 1
    xSemaphoreGive(sysState.mutex);
      // Use thresholds to determine if a directional move has been made.
      // Horizontal movement: left/right changes column.
    if (jx > JOYX_THRESHOLD_LEFT_MENU) {  // move left
        if (selection % 2 == 1) {  // currently right column
          selection -= 1;
        }
    } else if (jx < JOYX_THRESHOLD_RIGHT_MENU) {  // move right
        if (selection % 2 == 0) {  // currently left column
          selection += 1;
        }
    }
      // Vertical movement: up/down changes row.
    if (jy < JOYY_THRESHOLD_UP_MENU) {  // move up
        if (selection >= 2) {  // bottom row
          selection -= 2;
        }
    } else if (jy > JOYY_THRESHOLD_DOWN_MENU) {  // move down
        if (selection < 2) {  // top row
          selection += 2;
        }
      }
    return selection;
}
  