#include "web_input.h"
#include "web_ui.h"
#include "gamepad_mapping.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <freertos/semphr.h>
// The SoftAP password is kept out of git (public repository).
#if __has_include("rc_secrets.h")
#include "rc_secrets.h"
#else
#error "Copy rc_secrets.example.h to rc_secrets.h (same folder) and set RC_AP_PASSWORD (8+ characters)."
#endif

namespace {
WebServer http(RC_HTTP_PORT);
WebSocketsServer websocket(RC_WEBSOCKET_PORT);
SemaphoreHandle_t mutex;
VirtualGamepad browser;
PageStatus status;
int control_client = -1;
uint32_t generation = 0, last_status_ms = 0;
bool sequence_seen = false;

bool decode(const uint8_t* bytes, size_t size, VirtualGamepad& output) {
    if (size != 26 || bytes[0] != 0x56 || bytes[1] != 1 || (bytes[2] & ~1U) || bytes[3]) return false;
    output.focused = (bytes[2] & 1U) != 0;
    output.sequence = rc_read_u16(bytes + 4);
    output.lx = rc_read_i16(bytes + 6); output.ly = rc_read_i16(bytes + 8);
    output.rx = rc_read_i16(bytes + 10); output.ry = rc_read_i16(bytes + 12);
    output.lt = bytes[14]; output.rt = bytes[15]; output.buttons = rc_read_u16(bytes + 16);
    output.kick_press = rc_read_u16(bytes + 18); output.dribbler_press = rc_read_u16(bytes + 20);
    output.reset_press = rc_read_u16(bytes + 22); output.line_press = rc_read_u16(bytes + 24);
    if (output.lx < -1000 || output.lx > 1000 || output.ly < -1000 || output.ly > 1000 ||
        output.rx < -1000 || output.rx > 1000 || output.ry < -1000 || output.ry > 1000 || (output.buttons & ~0x7FU)) return false;
    // Unfocused messages may only revoke control, never carry movement/actions.
    if (!output.focused && (output.lx || output.ly || output.rx || output.ry || output.lt || output.rt || output.buttons)) return false;
    return true;
}

void event(uint8_t client, WStype_t type, uint8_t* bytes, size_t size) {
    if (type == WStype_CONNECTED) {
        if (control_client >= 0) {
            websocket.sendTXT(client, "{\"error\":\"control_busy\"}"); websocket.disconnect(client); return;
        }
        control_client = client; sequence_seen = false;
        xSemaphoreTake(mutex, portMAX_DELAY);
        browser = VirtualGamepad{}; browser.connected = true; browser.generation = ++generation;
        xSemaphoreGive(mutex);
    } else if (type == WStype_DISCONNECTED && control_client == client) {
        control_client = -1; sequence_seen = false;
        xSemaphoreTake(mutex, portMAX_DELAY); browser.connected = false; browser.valid = false; xSemaphoreGive(mutex);
    } else if (type == WStype_BIN && control_client == client) {
        VirtualGamepad candidate;
        if (!decode(bytes, size, candidate)) return;
        xSemaphoreTake(mutex, portMAX_DELAY);
        if (sequence_seen && !rc_sequence_newer(candidate.sequence, browser.sequence)) { xSemaphoreGive(mutex); return; }
        if (sequence_seen) {
            const uint16_t deltas[] = {static_cast<uint16_t>(candidate.kick_press - browser.kick_press),
                static_cast<uint16_t>(candidate.dribbler_press - browser.dribbler_press),
                static_cast<uint16_t>(candidate.reset_press - browser.reset_press),
                static_cast<uint16_t>(candidate.line_press - browser.line_press)};
            for (const uint16_t delta : deltas) if (delta > RC_EVENT_MAX_ADVANCE) { xSemaphoreGive(mutex); return; }
        }
        candidate.stop_press = browser.stop_press;
        qualifyReset(candidate, browser);
        if (candidate.focused && (candidate.buttons & VG_BACK) && !(browser.buttons & VG_BACK)) ++candidate.stop_press;
        candidate.generation = browser.generation; candidate.received_ms = millis();
        candidate.valid = true; candidate.connected = true; browser = candidate; sequence_seen = true;
        xSemaphoreGive(mutex);
    }
}

String quoted(const char* text) {
    String result = "\"";
    while (*text) {
        const unsigned char c = static_cast<unsigned char>(*text++);
        if (c == '\"' || c == '\\') result += '\\';
        if (c >= 32) result += static_cast<char>(c);
    }
    result += '\"'; return result;
}

void pushStatus() {
    PageStatus copy;
    xSemaphoreTake(mutex, portMAX_DELAY); copy = status; xSemaphoreGive(mutex);
    const char* owner = copy.owner == InputOwner::Gamepad ? "gamepad" : copy.owner == InputOwner::Browser ? "browser" : "none";
    char json[1000];
    snprintf(json, sizeof(json),
        "{\"owner\":\"%s\",\"operatorStop\":%s,\"robotLink\":%s,\"handover\":%s,"
        "\"flags\":%u,\"fault\":%u,\"heading\":%.2f,\"targetHeading\":%.2f,"
        "\"lineX\":%d,\"lineY\":%d,\"lineFlags\":%u,\"commandFlags\":%u,"
        "\"pairingSeconds\":%lu,\"gamepadConnected\":%s,\"gamepadApproved\":%s,\"pairingSaved\":%s,"
        "\"gamepadModel\":%s,\"gamepadAddress\":%s,\"firmware\":\"%u.%u.%u\",\"uartErrors\":%lu}",
        owner, copy.operator_stop ? "true" : "false", copy.robot_link ? "true" : "false", copy.handover ? "true" : "false",
        copy.robot.flags, copy.robot.fault, copy.robot.heading / 100.0, copy.robot.target_heading / 100.0,
        copy.robot.line_x, copy.robot.line_y, copy.robot.line_flags, copy.command_flags,
        static_cast<unsigned long>(copy.pairing_seconds), copy.gamepad_connected ? "true" : "false",
        copy.gamepad_approved ? "true" : "false", copy.pairing_saved ? "true" : "false",
        quoted(copy.gamepad_model).c_str(), quoted(copy.gamepad_address).c_str(),
        copy.robot.firmware_major, copy.robot.firmware_minor, copy.robot.firmware_patch,
        static_cast<unsigned long>(copy.robot.uart_errors));
    websocket.sendTXT(static_cast<uint8_t>(control_client), json);
}
}

namespace WebInput {
void begin() {
    mutex = xSemaphoreCreateMutex(); configASSERT(mutex);
    WiFi.mode(WIFI_AP);
    uint8_t mac[6]; WiFi.softAPmacAddress(mac);
    char ssid[40]; snprintf(ssid, sizeof(ssid), "%s%02X%02X", RC_AP_SSID_PREFIX, mac[4], mac[5]);
    const IPAddress address(192, 168, 4, 1);
    const bool ip_ok = WiFi.softAPConfig(address, address, IPAddress(255, 255, 255, 0));
    const bool ap_ok = WiFi.softAP(ssid, RC_AP_PASSWORD, RC_AP_CHANNEL, false, 2);
    Serial.printf("RC AP: %s http://192.168.4.1/ (%s)\n", ssid, ip_ok && ap_ok ? "ready" : "failed");
    http.on("/", HTTP_GET, [] { http.sendHeader("Cache-Control", "no-store"); http.send_P(200, "text/html; charset=utf-8", WEB_INDEX, sizeof(WEB_INDEX)-1); });
    // no-store on every asset: after re-flashing, a reload must never run a cached old controls.js.
    http.on("/style.css", HTTP_GET, [] { http.sendHeader("Cache-Control", "no-store"); http.send_P(200, "text/css; charset=utf-8", WEB_STYLE, sizeof(WEB_STYLE)-1); });
    http.on("/controls.js", HTTP_GET, [] { http.sendHeader("Cache-Control", "no-store"); http.send_P(200, "text/javascript; charset=utf-8", WEB_CONTROLS, sizeof(WEB_CONTROLS)-1); });
    http.onNotFound([] { http.send(404, "text/plain; charset=utf-8", "Not found"); });
    http.begin(); websocket.begin(); websocket.onEvent(event);
}
void service() {
    http.handleClient(); websocket.loop();
    const uint32_t now = millis();
    if (control_client >= 0 && now - last_status_ms >= 1000U / RC_TELEMETRY_HZ) {
        last_status_ms = now; pushStatus();
    }
}
VirtualGamepad snapshot() {
    VirtualGamepad copy;
    xSemaphoreTake(mutex, portMAX_DELAY); copy = browser; xSemaphoreGive(mutex); return copy;
}
void publish(const PageStatus& next) {
    xSemaphoreTake(mutex, portMAX_DELAY); status = next; xSemaphoreGive(mutex);
}
}
