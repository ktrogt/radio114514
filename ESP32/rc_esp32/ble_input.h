#pragma once
#include "virtual_gamepad.h"
namespace BleInput {
void begin();
void poll(uint32_t now);
void disconnect();
const VirtualGamepad& snapshot();
const char* model();
}
