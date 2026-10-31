#ifndef RC_SHARED_CONFIG_H
#define RC_SHARED_CONFIG_H

/* Shared, dependency-free limits. Translation is per-mille of match full scale. */
/* Doubled from 400/600/200 on the user's request (2026-10-13); 1000 is the match's full scale. */
#define RC_TRANSLATION_NORMAL 800
#define RC_TRANSLATION_BOOST 1000
#define RC_TRANSLATION_SLOW 400
/* Full-deflection turn values, clockwise positive (doubled 2026-10-13). Since 2026-10-14
 * the F446 maps turn/RC_TURN_* to a heading angle, so only the ratio matters now. */
#define RC_TURN_NORMAL 18000
#define RC_TURN_BOOST 27000
#define RC_TURN_SLOW 9000
/* Turn-axis deadzone for every input source (keyboard, phone, gamepad), per mille of full
 * deflection. Wider than the 120 move deadzone so sliding the phone throttle doesn't turn. */
#define RC_TURN_DEADZONE_PERMILLE 250
/* PID output limits relative to the match firmware's +/-800 correction. Doubled with the
 * turn rates (was 320/480/160) so the robot can follow them; boost is capped at the match's 800. */
#define RC_CORRECTION_NORMAL 640
#define RC_CORRECTION_BOOST 800
#define RC_CORRECTION_SLOW 320
#define RC_COMMAND_HZ 50U
#define RC_TELEMETRY_HZ 10U
#define RC_OWNER_CHANGE_OFF_MS 100U
#define RC_SOURCE_TIMEOUT_MS 200U
#define RC_ESP_TELEMETRY_TIMEOUT_MS 500U
/* Line assist state at boot and after an owner change (1 = on). Off until the line
 * sensor thresholds are tuned for the field: side sensors falsely report lines (2026-10-13). */
#define RC_LINE_ASSIST_DEFAULT 0
/* User-editable ESP defaults. The Wi-Fi password is NOT here (this repository is public):
 * it lives in ESP32/rc_esp32/rc_secrets.h, which git ignores (template: rc_secrets.example.h). */
#define RC_AP_SSID_PREFIX "GC-RC-"
#define RC_AP_CHANNEL 6U
#define RC_HTTP_PORT 80U
#define RC_WEBSOCKET_PORT 81U

#endif
