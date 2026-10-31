#pragma once
#include <stdint.h>

// begin() runs during setup; other public methods run on the control task.
// BT policy changes and persistence run on BTstack.
namespace Pairing {
void begin();
bool telemetry(uint16_t request, uint32_t now); // True only when a new window opens.
void tick(uint32_t now);
bool accept(const uint8_t address[6], uint32_t now);
void reject(const uint8_t address[6]);
void connected(bool value);
bool window(uint32_t now);
uint32_t remaining(uint32_t now);
bool approved();
bool saved();
void address(char out[18]);
}
