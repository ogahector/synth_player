#include <bitset>
#include <sig_gen.h>

extern bool doomLoadingShown;

std::bitset<4> readCols();

void setRow(uint8_t rowIdx);

void scanKeysTask(void * pvParameters);

void updateNotesPlayedFromCANTX(int index, uint8_t TX_Message[8]);