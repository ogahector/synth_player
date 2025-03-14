#pragma once

#include <bitset>
#include <sig_gen.h>
#include <array>


std::bitset<4> readCols();

void setRow(uint8_t rowIdx);

void scanKeysTask(void * pvParameters);

inline void updateNotesMaster(uint8_t RX_Message[8]);

std::array<int, 2> joystickRead();