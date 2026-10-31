#include "pairing.h"
#include <Arduino.h>
#include <Preferences.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
extern "C" {
#include <btstack.h>
#include <bt/uni_bt.h>
#include <bt/uni_bt_allowlist.h>
#include <uni_hid_device.h>
}

namespace {
struct Policy { uint8_t address[6] = {}; bool approved = false, window = false, connected = false; uint32_t save = 0; };
Policy policy;
SemaphoreHandle_t policy_mutex;
QueueHandle_t rejections;
btstack_context_callback_registration_t registration{};
uint32_t deadline = 0;
uint16_t previous_request = 0;
bool request_seen = false, storage_ok = true;
// BTstack LE units are 10 ms. Request BTstack 1.6.1's own default (720 ms) and
// refuse peer updates that would stretch loss detection beyond 1000 ms.
constexpr uint16_t kLeSupervisionTimeout = 72;
constexpr uint16_t kLeSupervisionTimeoutMax = 100;

void apply(void*) {
    // Public BTstack GAP APIs, on its thread and before discovery is enabled.
    // Preserve the 1.6.1 implementation defaults (including CE lengths of zero);
    // change only supervision timeout. gap.h's CE-length defaults are outdated.
    gap_set_connection_parameters(0x0060, 0x0030, 0x0008, 0x0018, 4, kLeSupervisionTimeout, 0, 0);
    le_connection_parameter_range_t range;
    gap_get_connection_parameter_range(&range);
    // Preserve interval/latency ranges and the 100 ms lower bound. Both L2CAP
    // and HCI peer update requests are checked against this public GAP range.
    range.le_supervision_timeout_max = kLeSupervisionTimeoutMax;
    gap_set_connection_parameter_range(&range);
    Policy copy;
    xSemaphoreTake(policy_mutex, portMAX_DELAY); copy = policy; xSemaphoreGive(policy_mutex);
    // Apply the allowlist and scan policy ONLY on BTstack's thread.
    uni_bt_allowlist_set_enabled(true);
    uni_bt_allowlist_remove_all();
    if (copy.approved) uni_bt_allowlist_add_addr(copy.address);
    uni_bt_allowlist_set_enabled(!copy.window);
    uni_bt_enable_new_connections_unsafe(!copy.connected && (copy.window || copy.approved));
    uint8_t rejected[6];
    while (xQueueReceive(rejections, rejected, 0) == pdTRUE) {
        auto* device = uni_hid_device_get_instance_for_address(rejected);
        if (device) uni_bt_disconnect_device_safe(uni_hid_device_get_idx_for_instance(device));
    }
    // Close raced/unexpected connections even if they have not produced a report.
    if (!copy.window) for (int i = 0; i < CONFIG_BLUEPAD32_MAX_DEVICES; ++i) {
        auto* device = uni_hid_device_get_instance_for_idx(i);
        if (device && (!copy.approved || memcmp(device->conn.btaddr, copy.address, 6) != 0))
            uni_bt_disconnect_device_safe(i);
    }
    static uint32_t saved_revision = 0;
    if (copy.save != saved_revision) {
        Preferences prefs;
        const bool ok = prefs.begin("gc-rc", false);
        const bool written = ok && prefs.putBytes("controller", copy.address, 6) == 6;
        if (ok) prefs.end();
        xSemaphoreTake(policy_mutex, portMAX_DELAY); storage_ok = written; xSemaphoreGive(policy_mutex);
        saved_revision = copy.save;
    }
}
void schedule() { btstack_run_loop_execute_on_main_thread(&registration); }
}

namespace Pairing {
void begin() {
    policy_mutex = xSemaphoreCreateMutex(); rejections = xQueueCreate(8, 6);
    configASSERT(policy_mutex && rejections);
    Preferences prefs;
    if (prefs.begin("gc-rc", true)) {
        if (prefs.getBytesLength("controller") == 6 && prefs.getBytes("controller", policy.address, 6) == 6)
            policy.approved = true;
        prefs.end();
    }
    registration.callback = apply; registration.context = nullptr;
    schedule();
}
bool telemetry(uint16_t request, uint32_t now) {
    if (!request_seen) { request_seen = true; previous_request = request; return false; }
    if (request == previous_request) return false;
    previous_request = request; deadline = now + 30000U;
    xSemaphoreTake(policy_mutex, portMAX_DELAY); policy.window = true; policy.connected = false; xSemaphoreGive(policy_mutex);
    schedule(); return true;
}
void tick(uint32_t now) {
    if (policy.window && static_cast<int32_t>(now - deadline) >= 0) {
        xSemaphoreTake(policy_mutex, portMAX_DELAY); policy.window = false; xSemaphoreGive(policy_mutex);
        schedule();
    }
}
bool accept(const uint8_t candidate[6], uint32_t now) {
    tick(now);
    bool accepted;
    xSemaphoreTake(policy_mutex, portMAX_DELAY);
    accepted = policy.window || (policy.approved && memcmp(candidate, policy.address, 6) == 0);
    if (accepted && policy.window) {
        memcpy(policy.address, candidate, 6); policy.approved = true; policy.window = false; ++policy.save;
    }
    if (accepted) policy.connected = true;
    xSemaphoreGive(policy_mutex);
    if (accepted) schedule();
    return accepted;
}
void reject(const uint8_t rejected[6]) { xQueueSend(rejections, rejected, 0); schedule(); }
void connected(bool value) {
    xSemaphoreTake(policy_mutex, portMAX_DELAY); policy.connected = value; xSemaphoreGive(policy_mutex); schedule();
}
bool window(uint32_t now) { return policy.window && static_cast<int32_t>(deadline - now) > 0; }
uint32_t remaining(uint32_t now) { return window(now) ? (deadline - now + 999U) / 1000U : 0; }
bool approved() { return policy.approved; }
bool saved() { xSemaphoreTake(policy_mutex, portMAX_DELAY); bool ok = storage_ok; xSemaphoreGive(policy_mutex); return ok; }
void address(char out[18]) {
    if (!policy.approved) { out[0] = '\0'; return; }
    snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X", policy.address[0], policy.address[1], policy.address[2],
             policy.address[3], policy.address[4], policy.address[5]);
}
}
