#include "ble_input.h"
#include "pairing.h"
#include "gamepad_mapping.h"
#include <Bluepad32.h>

namespace {
ControllerPtr controller = nullptr;
VirtualGamepad input;
char model_name[80] = {};
uint16_t previous_buttons = 0;
uint32_t generation = 0;
int16_t axis(int32_t value, bool invert = false) {
    value = constrain(value, -512, 511);
    const int32_t normalized = value * 1000 / (value < 0 ? 512 : 511);
    return static_cast<int16_t>(invert ? -normalized : normalized);
}
uint8_t trigger(int32_t value) { return static_cast<uint8_t>((constrain(value, 0, 1023) * 255 + 511) / 1023); }
void onConnect(ControllerPtr candidate) {
    const auto properties = candidate->getProperties();
    // onConnect can precede the first report, so isGamepad() is not usable here.
    if (controller || properties.type >= Controller::CONTROLLER_TYPE_GenericKeyboard ||
        !Pairing::accept(properties.btaddr, millis())) {
        Pairing::reject(properties.btaddr); return;
    }
    controller = candidate; input = VirtualGamepad{};
    input.connected = true; input.focused = true; input.generation = ++generation;
    previous_buttons = 0;
    candidate->getModelName().toCharArray(model_name, sizeof(model_name));
}
void onDisconnect(ControllerPtr candidate) {
    if (candidate != controller) return;
    controller = nullptr; input.connected = false; input.valid = false;
    model_name[0] = '\0'; Pairing::connected(false);
}
}
namespace BleInput {
void begin() {
    BP32.setup(onConnect, onDisconnect);
    BP32.enableVirtualDevice(false);
    BP32.enableBLEService(false);
}
void disconnect() {
    if (controller) {
        const auto properties = controller->getProperties(); Pairing::reject(properties.btaddr);
        controller = nullptr; input.connected = false; input.valid = false; model_name[0] = '\0';
    }
}
void poll(uint32_t now) {
    BP32.update();
    if (!controller) return;
    input.connected = controller->isConnected();
    if (!input.connected) { input.valid = false; return; }
    if (!controller->hasData()) return;
    if (!controller->isGamepad()) { disconnect(); Pairing::connected(false); return; }
    const auto previous = input;
    input.lx = axis(controller->axisX()); input.ly = axis(controller->axisY(), true);
    input.rx = axis(controller->axisRX()); input.ry = axis(controller->axisRY(), true);
    input.lt = trigger(controller->brake()); input.rt = trigger(controller->throttle());
    uint16_t buttons = 0;
    // Kick on A or ZR/RT (user, 2026-10-15); R1/RB stays boost.
    if (controller->a() || controller->r2()) buttons |= VG_A;
    if (controller->b()) buttons |= VG_B;
    if (controller->x()) buttons |= VG_X;
    if (controller->y()) buttons |= VG_Y;
    if (controller->l1()) buttons |= VG_LB;
    if (controller->r1()) buttons |= VG_RB;
    if (controller->miscSelect()) buttons |= VG_BACK;
    const uint16_t rising = buttons & static_cast<uint16_t>(~previous_buttons);
    if (rising & VG_A) ++input.kick_press;
    if (rising & VG_B) ++input.dribbler_press;
    if (rising & VG_Y) ++input.reset_press;
    if (rising & VG_X) ++input.line_press;
    if (rising & VG_BACK) ++input.stop_press;
    qualifyReset(input, previous);
    input.buttons = buttons; previous_buttons = buttons;
    // Keep the last-report timestamp for diagnostics, not a BLE input timeout.
    ++input.sequence; input.received_ms = now; input.valid = true;
}
const VirtualGamepad& snapshot() { return input; }
const char* model() { return model_name; }
}
