#pragma once

//Display update header
// #include <Arduino.h>
#include <string>

std::string inputToKeyString(uint32_t inputs);

void displayUpdateTask(void* vParam);