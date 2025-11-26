#include "vl53lxx-v2.hpp"

#include "config.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Hardware
{
    namespace Laser
    {
        VL53L0X::VL53L0X(const i2c_port_t i2c_port, const gpio_num_t sda_pin,
                         const gpio_num_t scl_pin, const uint8_t device_address)
            : i2c_port_(i2c_port), sda_pin_(sda_pin), scl_pin_(scl_pin),
              device_address_(device_address), initialized_(false)
        {
            if (!initI2C())
            {
                ESP_LOGE(LOG_TAG, "Failed to initialize I2C");
                return;
            }

            if (!initSensor())
            {
                ESP_LOGE(LOG_TAG, "Failed to initialize VL53L0X sensor");
                return;
            }

            initialized_ = true;
            ESP_LOGI(LOG_TAG, "VL53L0X configured successfully");
        }

        VL53L0X::~VL53L0X()
        {
            if (initialized_)
                i2c_driver_delete(i2c_port_);
        }

        bool VL53L0X::initI2C()
        {
            i2c_config_t conf = {};
            conf.mode = I2C_MODE_MASTER;
            conf.sda_io_num = sda_pin_;
            conf.scl_io_num = scl_pin_;
            conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
            conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
            conf.master.clk_speed = 400000; // 400 kHz

            esp_err_t err = i2c_param_config(i2c_port_, &conf);
            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "I2C param config failed: %s", esp_err_to_name(err));
                return false;
            }

            err = i2c_driver_install(i2c_port_, conf.mode, 0, 0, 0);
            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "I2C driver install failed: %s", esp_err_to_name(err));
                return false;
            }

            ESP_LOGI(LOG_TAG, "I2C initialized on port %d (SDA: %d, SCL: %d)",
                     i2c_port_, sda_pin_, scl_pin_);
            return true;
        }

        bool VL53L0X::initSensor()
        {
            // Verify device ID
            uint8_t model_id = 0;
            if (!readRegister(REG_IDENTIFICATION_MODEL_ID, model_id))
            {
                ESP_LOGE(LOG_TAG, "Failed to read model ID");
                return false;
            }

            if (model_id != EXPECTED_MODEL_ID)
            {
                ESP_LOGE(LOG_TAG, "Invalid model ID: 0x%02X (expected 0x%02X)",
                         model_id, EXPECTED_MODEL_ID);
                return false;
            }

            ESP_LOGI(LOG_TAG, "VL53L0X detected (Model ID: 0x%02X)", model_id);

            // Basic initialization - sensor is ready for single-shot measurements
            vTaskDelay(pdMS_TO_TICKS(10));

            return true;
        }

        float VL53L0X::MeasureDistance(const uint32_t timeout_ms)
        {
            if (!initialized_)
            {
                ESP_LOGE(LOG_TAG, "Sensor not initialized");
                return -1.0f;
            }

            // Start single-shot measurement
            if (!writeRegister(REG_SYSRANGE_START, SYSRANGE_START_SINGLE))
            {
                ESP_LOGE(LOG_TAG, "Failed to start measurement");
                return -1.0f;
            }

            // Wait for measurement to complete
            uint32_t start_time = xTaskGetTickCount();
            uint8_t status = 0;

            while (true)
            {
                if (!readRegister(REG_RESULT_RANGE_STATUS, status))
                {
                    ESP_LOGE(LOG_TAG, "Failed to read status register");
                    return -1.0f;
                }

                // Check if measurement is ready (bit 0 of status register)
                if ((status & 0x01) != 0)
                {
                    break;
                }

                if ((xTaskGetTickCount() - start_time) > pdMS_TO_TICKS(timeout_ms))
                {
                    ESP_LOGW(LOG_TAG, "Measurement timeout");
                    return -1.0f;
                }

                vTaskDelay(pdMS_TO_TICKS(10));
            }

            // Read distance result (registers 0x14 + 10 and 0x14 + 11)
            uint8_t buffer[2];
            if (!readRegisters(REG_RESULT_RANGE_STATUS + 10, buffer, 2))
            {
                ESP_LOGE(LOG_TAG, "Failed to read distance");
                return -1.0f;
            }

            uint16_t distance_mm = (buffer[0] << 8) | buffer[1];
            float distance_cm = distance_mm / 10.0f;

            // Validate reading (VL53L0X range: up to 200cm typically)
            if (distance_mm == 0 || distance_mm == 8191)
            {
                ESP_LOGW(LOG_TAG, "Invalid reading: %d mm", distance_mm);
                return -1.0f;
            }

            if (distance_cm >= Config::DISTANCE_THRESHOLD_CM)
            {
                ESP_LOGW(LOG_TAG, "Distance threshold achieved or surpassed: %.2f cm over the threshold",
                         (distance_cm - Config::DISTANCE_THRESHOLD_CM));
            }
            else
            {
                ESP_LOGI(LOG_TAG, "Distance: %.2f cm (%d mm)", distance_cm, distance_mm);
            }

            return distance_cm;
        }

        bool VL53L0X::writeRegister(const uint8_t reg, const uint8_t value)
        {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (device_address_ << 1) | I2C_MASTER_WRITE, true);
            i2c_master_write_byte(cmd, reg, true);
            i2c_master_write_byte(cmd, value, true);
            i2c_master_stop(cmd);

            esp_err_t err = i2c_master_cmd_begin(i2c_port_, cmd, pdMS_TO_TICKS(1000));
            i2c_cmd_link_delete(cmd);

            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "I2C write failed: %s", esp_err_to_name(err));
                return false;
            }

            return true;
        }

        bool VL53L0X::readRegister(const uint8_t reg, uint8_t &value)
        {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (device_address_ << 1) | I2C_MASTER_WRITE, true);
            i2c_master_write_byte(cmd, reg, true);
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (device_address_ << 1) | I2C_MASTER_READ, true);
            i2c_master_read_byte(cmd, &value, I2C_MASTER_NACK);
            i2c_master_stop(cmd);

            esp_err_t err = i2c_master_cmd_begin(i2c_port_, cmd, pdMS_TO_TICKS(1000));
            i2c_cmd_link_delete(cmd);

            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "I2C read failed: %s", esp_err_to_name(err));
                return false;
            }

            return true;
        }

        bool VL53L0X::readRegisters(const uint8_t reg, uint8_t *buffer, const size_t length)
        {
            i2c_cmd_handle_t cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (device_address_ << 1) | I2C_MASTER_WRITE, true);
            i2c_master_write_byte(cmd, reg, true);
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (device_address_ << 1) | I2C_MASTER_READ, true);

            if (length > 1)
                i2c_master_read(cmd, buffer, length - 1, I2C_MASTER_ACK);
            i2c_master_read_byte(cmd, buffer + length - 1, I2C_MASTER_NACK);
            i2c_master_stop(cmd);

            esp_err_t err = i2c_master_cmd_begin(i2c_port_, cmd, pdMS_TO_TICKS(1000));
            i2c_cmd_link_delete(cmd);

            if (err != ESP_OK)
            {
                ESP_LOGE(LOG_TAG, "I2C read multiple failed: %s", esp_err_to_name(err));
                return false;
            }

            return true;
        }
    }
}