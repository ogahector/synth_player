#pragma once

extern const uint32_t stepSizes[];

void decodeTask(void * pvParameters);

void transmitTask (void * pvParameters);

inline void updateNotesPlayedFromCANTX(uint8_t RX_Message[8]);