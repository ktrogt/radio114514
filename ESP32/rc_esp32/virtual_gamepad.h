#pragma once
#include <stdint.h>

enum VirtualButton : uint16_t {
    VG_A = 1U << 0, VG_B = 1U << 1, VG_X = 1U << 2, VG_Y = 1U << 3,
    VG_LB = 1U << 4, VG_RB = 1U << 5, VG_BACK = 1U << 6
};

// Source adapters normalize axes and retain press counters across snapshots.
struct VirtualGamepad {
    int16_t lx = 0, ly = 0, rx = 0, ry = 0;
    uint8_t lt = 0, rt = 0;
    uint16_t buttons = 0;
    uint16_t kick_press = 0, dribbler_press = 0, reset_press = 0, line_press = 0;
    uint16_t reset_neutral_press = 0; // Qualified at receipt, before snapshots can coalesce.
    // Local Back rising-edge counter; the 26-byte wire format is unchanged.
    uint16_t stop_press = 0, sequence = 0;
    uint32_t received_ms = 0, generation = 0;
    bool valid = false, focused = false, connected = false;
};

enum class InputOwner : uint8_t { None, Browser, Gamepad };
