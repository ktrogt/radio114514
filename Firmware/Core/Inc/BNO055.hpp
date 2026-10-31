#ifndef BNO055_HPP_
#define BNO055_HPP_
#include "stm32f4xx_hal.h"
struct EULAR { float x, y, z; };
struct QUATERNION { float x, y, z, w; };

/* Match quaternion conversion, checked transactions on the real HAL handle. */
class BNO055 {
    I2C_HandleTypeDef *i2c_;
    uint8_t address_;
public:
    BNO055(I2C_HandleTypeDef *i2c, uint8_t address) : i2c_(i2c), address_(address) {}
    bool init_imu();
    bool read_euler(EULAR& e);
};
#endif
