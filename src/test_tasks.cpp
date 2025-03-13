#include <bitset>
#include <array>
#include <knob.h>
#include <Arduino.h>
#include <STM32FreeRTOS.h>
#include <globals.h>
#include <menu.h>
#include <waves.h>
#include <home.h>
#include <U8g2lib.h>
#include <scanKeysTask.h>
// A test version of scanKeysTask that simulates worst-case processing.
// This version does not block and generates a key press for every key.

#ifdef TEST_SCANKEYS
std::bitset<32> inputs;
Knob K3 = Knob(0,8);//Volume Knob
Knob K2 = Knob(-4,4);//Octave Knob

bool muteReleased = true;
bool slaveReleased = true;
bool menuButton = false;
bool joystickButton = false;
std::array<int, 2> joystickValues;
std::bitset<4> cols;
std::bitset<4> row_cols;
static bool toggle = false;
uint8_t TX_Message[8] = {0};//Message sent over CAN
std::bitset<5> activityList;


void testScanKeys(int state_choice) {
    setRow(6);
    delayMicroseconds(3);
    row_cols = readCols();

    if (!row_cols[0] && menuButton){
      menuButton = false;
      activityList.reset();
      if (toggle) {
        activityList[0] = 1;
      } else {
        activityList[1] = 0;
      }
      toggle = !toggle;  // flip toggle for next press
    }
    else if (row_cols[0]) menuButton = true;

    switch (state_choice)
    {
      case 0:
        for (int i = 0; i < 7; i++){//Read rows
          setRow(i);
          delayMicroseconds(3);
          cols = readCols();
          for (int j = 0; j < 4; j++) inputs[4*i + j] = 0;
        }
        for (int i = 0; i < 12; i++){//Checks all keys
            TX_Message[0] = 'P';
            TX_Message[1] =  4;
            TX_Message[2] = i;
            TX_Message[3] = 255;
            xQueueSend( msgOutQ, TX_Message, 0);//Sends via CAN
        }
        K3.update(inputs[12],inputs[13]);//Volume adjustment
        K2.update(inputs[14], inputs[15]);//Octave Adjustment

        if(!inputs[21] && muteReleased) {
          muteReleased = false;
        }
        else if (inputs[21]) muteReleased = true;
  
      //Toggles slave (Knob 2 Press)
        if(!inputs[20] && slaveReleased) {
          slaveReleased = false;

        }
        else if (inputs[20]) slaveReleased = true;
        break;

      case 1:
        joystickValues = joystickRead();
        setRow(5);
        delayMicroseconds(3);
        row_cols = readCols();
        if(1 && joystickButton) {
          joystickButton = false;
        }
        else if (0) joystickButton = true;
        break;
      
      case 2:
        joystickValues = joystickRead();
        setRow(5);
        delayMicroseconds(3);
        row_cols = readCols();
        break;
      case 3:
        joystickValues = joystickRead();
        setRow(5);
        delayMicroseconds(3);
        row_cols = readCols();
        // Treat the joystick press as home button press
        if(1 && joystickButton) {
          joystickButton = false;
          menuButton = false;
          toggle=false;
          activityList[0]=1;
        }
        else if (0) {
          joystickButton = true;
          menuButton = true;
        }
        break;
      default:
        break;
    }
}
#endif

#ifdef TEST_DISPLAYUPDATE
int localActivity = -1;
int localJoystickDir = 0;
static int previousActivity = -1;
bool doomLoadingShown1=false;

int currentMenuIndex1=2;

int cameraOffsetX = 0;
int cameraOffsetY = 0;

int selection1 = 0;  
const int boxWidth = u8g2.getDisplayWidth() / 2 - 2;
const int boxHeight = u8g2.getDisplayHeight() / 2 - 2;
const int margin = 2; // margin within each box for drawing the waveform
const int spacing = 1;

const int barWidth = 6;    // Width of each volume bar
const int barHeight = 8;  // Height of the volume bar
const int numBars = 7;     // Maximum volume level (0-7)
const int barStartX = 62; // X position where the bars start
const int barStartY = 12;
#define MAX_BULLETS 10

struct Bullet {
    int x, y;
    bool active;  // Whether the bullet is currently active
};

Bullet bullets1[MAX_BULLETS];

void initBullets1() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets1[i].active = false;  // Set all bullets as inactive initially
    }
}

// Shoot a bullet from the center of the gun
void shootBullet1() {
    // Find an inactive bullet slot
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets1[i].active) {
            bullets1[i].x = 69;   // Start from center-bottom of the screen (adjust if needed)
            bullets1[i].y = 19;   // Just above the gun
            bullets1[i].active = true;
            break;
        }
    }
}

// Move and render bullets
void updateBullets1() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets1[i].active) {
            bullets1[i].y -= 1;  // Move bullet upwards (adjust speed if needed)
            bullets1[i].x -= 1;
            // Check if the bullet has moved off-screen
            if (bullets1[i].y < 0) {
                bullets1[i].active = false;  // Deactivate the bullet
            } else {
                // Draw the bullet as a pixel
                u8g2.drawPixel(bullets1[i].x, bullets1[i].y);
            }
        }
    }
}

void testDisplayUpdate(int state_choice)
{
  
  
switch (state_choice)
    {
      case 1:
        localActivity = 1;
        break;
      case 2:
        localActivity = 2;
        break;
      case 3:
        localActivity = 3;
        break;
      case 0:
        localActivity = 0;
        break;
      default:
        localActivity = -1;
        break;
    }

    if (localActivity == 2)
    {
      // Transitioning into doom state: if we weren't in doom previously, trigger loading screen.
      if (previousActivity != 2) {
        doomLoadingShown1 = false; // Show loading screen
      }
      else {
        doomLoadingShown1 = true;  // Already in doom; no need to show the loading screen again.
      }
    }
    else {
      // For non-doom activities, reset the flag so that if we return to doom, the loading screen appears.
      doomLoadingShown1 = true;
    }
    switch (localActivity)
    {
      case 1:{
      u8g2.clearBuffer();
      int direction = 1;
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
      animateMenuTransition(currentMenuIndex1, direction);
        switch (currentMenuIndex1)
          {
              case 2:
                activityList[2] = 1;
                  break;
              case 3: 
                  activityList[3] = 1;
                  break;
              default:
                  break;
          }
      }
      u8g2.sendBuffer();
        break;
      case 2:{
        u8g2.clearBuffer();
        u8g2.setDrawColor(1);  // 1 = white, 0 = black
      
          // Render the loading image (from doomLoadScreen)
        for (size_t i = 0; i < numLoadOnes; i++) {
          u8g2.drawPixel(doomLoadScreen[i].col, doomLoadScreen[i].row);
        }
        u8g2.sendBuffer();
        cameraOffsetX = 0;
        cameraOffsetY = 0;
          // Wait for 1 second
        
      
        // Always display the start scene
      u8g2.clearBuffer();
        
    
        // Shoot when joystick is pressed
      int jx, jy;
      bool shoot;
      jx = 500;
      jy = 500;
      shoot = 1;
      
      if (shoot) {
        shootBullet1();
      }
    
      // Update and render bullets
      updateBullets1();
      const int centerValX = 460;
      const int centerValY = 516;
      int deltaX = (jx - centerValX) / 150;  
      int deltaY = (jy - centerValY) / 150;
      cameraOffsetX -= deltaX;
      cameraOffsetY += deltaY;
    
      for (size_t i = 0; i < numEnemy; i++) {
        u8g2.drawPixel(doomEnemy[i].col + cameraOffsetX, doomEnemy[i].row + cameraOffsetY);
      }
      u8g2.setDrawColor(0);
      for (size_t i = 0; i < numGunOutline; i++) {
          u8g2.drawPixel(gunOutline[i].col + 10, gunOutline[i].row); // This uses the current draw color (assumed white)
      }
      u8g2.setDrawColor(1);
    
      // Then draw the gun pixel itself (which will not remove the outline)
      for (size_t i = 0; i < numGun; i++) {
        u8g2.drawPixel(doomGun[i].col + 10, doomGun[i].row); // This uses the current draw color (assumed white)
      }
      u8g2.sendBuffer();
        break;
    }
      case 3:{
        u8g2.clearBuffer();
  
      // Draw each of the four waveform boxes
      for (int i = 0; i < 4; i++) {
          int col = i % 2;
          int row = i / 2;
          int x = col * (boxWidth + spacing) + spacing;
          int y = row * (boxHeight + spacing) + spacing;
  
    
          // If this box is selected, highlight it by filling the box.
          if (i == selection1) {
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
        
        // Read joystick data (replace with your actual method to get joystick values)
      int jx = 500;         // e.g., -1 to 1
      int jy = 500;          // e.g., -1 to 1
        // Use thresholds to determine if a directional move has been made.
        // Horizontal movement: left/right changes column.
      if (jx > 650) {  // move left
          if (selection1 % 2 == 1) {  // currently right column
            selection1 -= 1;
          }
      } else if (jx < 300) {  // move right
          if (selection1 % 2 == 0) {  // currently left column
            selection1 += 1;
          }
      }
        // Vertical movement: up/down changes row.
      if (jy < 300) {  // move up
          if (selection1 >= 2) {  // bottom row
            selection1 -= 2;
          }
      } else if (jy > 650) {  // move down
          if (selection1 < 2) {  // top row
            selection1 += 2;
          }
        }
        break;
      }
      case 0: {
      u8g2.clearBuffer();         // clear the internal memory
      u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
      // u8g2.drawStr(2,10,"Current Stack Size: ");  // write something to the internal memory
      u8g2.drawStr(2, 10, "Octave: ");
      u8g2.setCursor(55,10);
      u8g2.print(sysState.Octave, DEC);
      u8g2.setCursor(75,10);
      if(sysState.slave) u8g2.print("Slave");
      else u8g2.print("Master");
  
      u8g2.drawStr(2, 20, "Volume: ");
      u8g2.setCursor(55,20);
      
      if (sysState.mute) u8g2.print("X");
      else u8g2.print(0, DEC); 
      
      for (int i = 0; i < volumeLevel+1; i++) {
        u8g2.drawBox(barStartX + i * (barWidth + 2), barStartY, barWidth, barHeight);  // Draw the bar
      }
      u8g2.setCursor(65,30);
      u8g2.print((char) sysState.RX_Message[0]);
      u8g2.print(sysState.RX_Message[1]);
      u8g2.print(sysState.RX_Message[2]);
      u8g2.print(sysState.RX_Message[3]);
      u8g2.drawStr(2, 30, inputToKeyString(sysState.inputs.to_ulong()).c_str());
      u8g2.sendBuffer();          // transfer internal memory to the display
      digitalToggle(LED_BUILTIN);
        break;
      }
      default:
        u8g2.clearBuffer();
        u8g2.setCursor(0, 10);
        u8g2.print("There was an error");
        u8g2.sendBuffer();
        break;
    }   
    previousActivity = localActivity;
}

#endif

