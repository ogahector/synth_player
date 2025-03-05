#include <bitset>

extern bool doomLoadingShown;

std::bitset<4> readCols();

void setRow(uint8_t rowIdx);

void scanKeysTask(void * pvParameters);