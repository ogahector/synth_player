#include <globals.h>
#include <U8g2lib.h>

const int barWidth = 6;    // Width of each volume bar
const int barHeight = 8;  // Height of the volume bar
const int numBars = 7;     // Maximum volume level (0-7)
const int barStartX = 62; // X position where the bars start
const int barStartY = 12;  // Y position where the bars are drawn

std::string inputToKeyString(uint32_t inputs)//Just gets key from input
{
  // Isolate first 12 LSB
  // inputs = (inputs << (32 - 12)) >> (32 - 12);
  int index;
  for(index = 0; index < 12; index++)
  {
    if(!((inputs >> index) & 1))
      break; 
  }

  switch(index){
    case 0: return "C";
    case 1: return "C#";
    case 2: return "D";
    case 3: return "D#";
    case 4: return "E";
    case 5: return "F";
    case 6: return "F#";
    case 7: return "G";
    case 8: return "G#";
    case 9: return "A";
    case 10: return "A#";
    case 11: return "B";
    default: return "";
  }
  return "";
}

void renderHome(){
    u8g2.clearBuffer();         // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
    // u8g2.drawStr(2,10,"Current Stack Size: ");  // write something to the internal memory
    u8g2.drawStr(2, 10, "Octave: ");
    u8g2.setCursor(55,10);
    xSemaphoreTake(sysState.mutex, portMAX_DELAY);
    u8g2.print(sysState.Octave, DEC);
    u8g2.setCursor(75,10);
    if(sysState.slave) u8g2.print("Slave");
    else u8g2.print("Master");

    u8g2.drawStr(2, 20, "Volume: ");
    u8g2.setCursor(55,20);
    
    if (sysState.mute) u8g2.print("X");
    else u8g2.print(sysState.Volume, DEC); 
    
    int volumeLevel = sysState.Volume;  // Get the current volume count (0-7)
    for (int i = 0; i < volumeLevel+1; i++) {
      u8g2.drawBox(barStartX + i * (barWidth + 2), barStartY, barWidth, barHeight);  // Draw the bar
    }
    u8g2.setCursor(65,30);
    u8g2.print((char) sysState.RX_Message[0]);
    u8g2.print(sysState.RX_Message[1]);
    u8g2.print(sysState.RX_Message[2]);
    u8g2.print(sysState.RX_Message[3]);
    u8g2.drawStr(2, 30, inputToKeyString(sysState.inputs.to_ulong()).c_str());
    xSemaphoreGive(sysState.mutex);
    u8g2.sendBuffer();          // transfer internal memory to the display
    digitalToggle(LED_BUILTIN);
}