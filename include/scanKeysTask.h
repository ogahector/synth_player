#pragma once

#include <bitset>
#include <sig_gen.h>
#include <array>


std::bitset<4> readCols();

void setRow(uint8_t rowIdx);

void scanKeysTask(void * pvParameters);

void updateNotesPlayedFromCANTX(int index, uint8_t TX_Message[8]);

std::array<int, 2> joystickRead();