#pragma once

extern const uint32_t stepSizes[];

void decodeTask(void * pvParameters);

void transmitTask (void * pvParameters);

void masterDecodeTask(void * pwParameters);