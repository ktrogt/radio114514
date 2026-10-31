#include "BNO055.hpp"
#include "rc_config_f446.h"
#include <cmath>

bool BNO055::init_imu() {
    uint8_t mode[] = {0x3d, 0x08}; // Original match IMU-mode write.
    return HAL_I2C_Master_Transmit(i2c_, static_cast<uint16_t>(address_ << 1),
                                  mode, sizeof(mode), RC_I2C_TIMEOUT_MS) == HAL_OK;
}

bool BNO055::read_euler(EULAR& e) {
    uint8_t reg = 0x20;
    uint8_t bytes[8] = {};
    if (HAL_I2C_Master_Transmit(i2c_, static_cast<uint16_t>(address_ << 1), &reg, 1,
                                RC_I2C_TIMEOUT_MS) != HAL_OK ||
        HAL_I2C_Master_Receive(i2c_, static_cast<uint16_t>(address_ << 1), bytes,
                               sizeof(bytes), RC_I2C_TIMEOUT_MS) != HAL_OK) return false;
    float values[4];
    for (unsigned i = 0; i < 4; ++i) {
        const uint16_t u = static_cast<uint16_t>(bytes[i*2]) |
                           static_cast<uint16_t>(static_cast<uint16_t>(bytes[i*2+1]) << 8);
        const int32_t signed_value = u <= 32767U ? static_cast<int32_t>(u) : static_cast<int32_t>(u)-65536;
        values[i] = static_cast<float>(signed_value) / 16384.0f;
    }
    QUATERNION q{values[1], values[2], values[3], values[0]};
    const float norm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    if (!std::isfinite(norm) || norm < 0.5f || norm > 1.5f) return false;
    constexpr float degrees = 180.0f / 3.14159265358979323846f;
    // Preserve the match's quaternion-to-roll/pitch/yaw equations.
    e.x = std::atan2(2.0f*(q.w*q.x+q.y*q.z), 1.0f-2.0f*(q.x*q.x+q.y*q.y))*degrees;
    const float sinp = 2.0f*(q.w*q.y-q.z*q.x);
    e.y = std::fabs(sinp) >= 1.0f ? std::copysign(90.0f,sinp) : std::asin(sinp)*degrees;
    e.z = std::atan2(2.0f*(q.w*q.z+q.x*q.y), 1.0f-2.0f*(q.y*q.y+q.z*q.z))*degrees;
    return std::isfinite(e.x) && std::isfinite(e.y) && std::isfinite(e.z);
}
