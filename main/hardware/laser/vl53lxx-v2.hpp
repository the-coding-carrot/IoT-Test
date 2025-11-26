#pragma once

#include "driver/i2c.h"

#include <cstdint>

namespace Hardware
{
    namespace Laser
    {
        class VL53L0X
        {
        public:
            VL53L0X(const i2c_port_t i2c_port, const gpio_num_t sda_pin, const gpio_num_t scl_pin, const uint8_t device_address = 0x29);

            ~VL53L0X();

            float MeasureDistance(const uint32_t timeout_ms = 500);

        private:
            static constexpr const char *LOG_TAG = "VL53L0X";

            // VL53L0X Register addresses
            static constexpr uint8_t REG_IDENTIFICATION_MODEL_ID = 0xC0;
            static constexpr uint8_t REG_SYSRANGE_START = 0x00;
            static constexpr uint8_t REG_RESULT_RANGE_STATUS = 0x14;
            static constexpr uint8_t SYSRANGE_START_SINGLE = 0x01;

            static constexpr uint8_t EXPECTED_MODEL_ID = 0xEE;

            i2c_port_t i2c_port_;
            gpio_num_t sda_pin_;
            gpio_num_t scl_pin_;
            uint8_t device_address_;
            bool initialized_;

            // Initialize I2C bus
            bool initI2C();

            // Initialize VL53L0X sensor
            bool initSensor();

            // Write a byte to a register
            bool writeRegister(const uint8_t reg, const uint8_t value);

            // Read a byte from a register
            bool readRegister(const uint8_t reg, uint8_t &value);

            // Read multiple bytes from a register
            bool readRegisters(const uint8_t reg, uint8_t *buffer, const size_t length);
        };
    }
}