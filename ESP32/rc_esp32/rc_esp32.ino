#include <Arduino.h>
#include <Bluepad32.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <esp_log.h>
#include "ble_input.h"
#include "gamepad_mapping.h"
#include "input_arbiter.h"
#include "pairing.h"
#include "uart_link.h"
#include "web_input.h"

static int discardLog(const char*, va_list) { return 0; }

static void controlTask(void*) {
    UartLink uart;
    InputArbiter arbiter;
    GamepadMapping mapping;
    OperatorStop stop;
    uart.begin();
    TickType_t wake = xTaskGetTickCount();
    uint32_t next_command = millis();
    bool stop_frame_pending = false;
    for (;;) {
        const uint32_t now = millis();
        const bool updated = uart.poll(now);
        if (updated) {
            const auto& telemetry = uart.telemetry();
            if (Pairing::telemetry(telemetry.pairing_request, now)) BleInput::disconnect();
            const bool stopped = (telemetry.flags & RC_STATUS_REARM_REQUIRED) ||
                !(telemetry.flags & RC_STATUS_RED_ENABLED) || !(telemetry.flags & RC_STATUS_WHEELS_ACTIVE) ||
                telemetry.fault != RC_FAULT_NONE;
            if (stopped && !(telemetry.flags & RC_STATUS_DRIBBLER_ACTIVE)) mapping.clearDribbler();
        }
        Pairing::tick(now); BleInput::poll(now);
        const auto browser = WebInput::snapshot();
        const auto& gamepad = BleInput::snapshot();
        stop.update(browser, 0, now); stop.update(gamepad, 1, now);
        if (stop.consumeActivation()) stop_frame_pending = true;
        const bool changed = arbiter.update(browser, gamepad, now);
        const VirtualGamepad* input = arbiter.owner() == InputOwner::Gamepad ? &gamepad :
                                     arbiter.owner() == InputOwner::Browser ? &browser : nullptr;
        if (changed) mapping.handover(input);
        if (stop.active()) mapping.clearDribbler();
        if (static_cast<int32_t>(now - next_command) >= 0) {
            const bool operator_stop = stop.active() || stop_frame_pending;
            const bool enabled = arbiter.ready(now) && !operator_stop;
            const auto command = mapping.command(input, enabled, !arbiter.ready(now));
            uart.send(command);
            stop_frame_pending = false;
            if (!enabled) arbiter.disabledFrameSent(now);
            next_command += 1000U / RC_COMMAND_HZ;
            // Skip missed slots rather than bursting old commands after an overrun.
            if (static_cast<int32_t>(now - next_command) >= 0) next_command = now + 1000U / RC_COMMAND_HZ;
            PageStatus status;
            status.owner = arbiter.owner(); status.operator_stop = operator_stop;
            status.robot_link = uart.fresh(now); status.robot = uart.telemetry();
            status.handover = input && !arbiter.ready(now); status.command_flags = command.flags;
            status.gamepad_connected = gamepad.connected; status.gamepad_approved = Pairing::approved();
            status.pairing_seconds = Pairing::remaining(now); status.pairing_saved = Pairing::saved();
            snprintf(status.gamepad_model, sizeof(status.gamepad_model), "%s", BleInput::model());
            Pairing::address(status.gamepad_address); WebInput::publish(status);
        }
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(5));
    }
}

void setup() {
    // IDF/Bluepad32 logs are suppressed before GPIO43/44 are assigned to UART1.
    // Package startup/ROM logs use UART0; no code here starts UART0/Serial0.
    esp_log_level_set("*", ESP_LOG_NONE); esp_log_set_vprintf(discardLog);
    Serial.begin(115200); // USB CDC with XIAO's default CDC-on-boot setting.
    // Stop the package's initial discovery before Wi-Fi initialization can block.
    BP32.enableNewBluetoothConnections(false);
    BleInput::begin(); Pairing::begin();
    WebInput::begin();
    const BaseType_t started = xTaskCreatePinnedToCore(controlTask, "rc-control", 8192, nullptr, 2, nullptr, 1);
    configASSERT(started == pdPASS);
}
void loop() { WebInput::service(); delay(1); }
