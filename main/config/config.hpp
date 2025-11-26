#pragma once

#include "driver/gpio.h"
#include "driver/i2c.h"

namespace Config
{
    // ──────────────────────────────
    // Application Info
    // ──────────────────────────────
    static constexpr const char *APP_NAME = "IoT Test"; // Application name
    static constexpr const char *APP_VERSION = "1.0.0"; // Application version

    // ──────────────────────────────
    // Laser Sensor (VL53L0X)
    // ──────────────────────────────
    static constexpr i2c_port_t VL53L0X_I2C_PORT = I2C_NUM_0;
    static constexpr gpio_num_t VL53L0X_SDA_PIN = GPIO_NUM_21; // Typical ESP32 I2C SDA
    static constexpr gpio_num_t VL53L0X_SCL_PIN = GPIO_NUM_22; // Typical ESP32 I2C SCL
    static constexpr uint32_t VL53L0X_TIMEOUT_MS = 200;
    static constexpr float DISTANCE_THRESHOLD_CM = 200.0f; // Max valid distance (cm)

    // ──────────────────────────────
    // Mailbox Detection Logic
    // ──────────────────────────────
    static constexpr float BASELINE_CM = 40.0f;     // Empty mailbox baseline (cm)
    static constexpr float TRIGGER_DELTA_CM = 2.0f; // Min change to detect occlusion (cm)
    static constexpr uint32_t HOLD_MS = 200;        // Occlusion hold time (ms)
    static constexpr uint32_t REFRACTORY_MS = 8000; // Refractory period after detection (ms)

    // ──────────────────────────────
    // Filtering
    // ──────────────────────────────
    static constexpr uint8_t FILTER_WINDOW = 3; // Median filter window size

    // ──────────────────────────────
    // MQTT Settings
    // ──────────────────────────────
    static constexpr const char *MQTT_BROKER_URI = "mqtt://192.168.1.100:1883"; // Broker URI
    static constexpr const char *MQTT_BASE_TOPIC = "home/mailbox";              // Base topic
    static constexpr const char *MQTT_CLIENT_ID = "mailbox-sensor-001";         // Client ID

    // ──────────────────────────────
    // Wi-Fi Settings
    // ──────────────────────────────
    static constexpr const char *CONN_SSID = "Test"; // Wi-Fi SSID
    static constexpr const char *PASSWORD = "Test";  // Wi-Fi password

    // ──────────────────────────────
    // Power Management
    // ──────────────────────────────
    static constexpr uint64_t DEEP_SLEEP_US = 5000000;       // Deep sleep duration (µs) - 5 seconds
    static constexpr uint64_t HEARTBEAT_INTERVAL_SEC = 3600; // Heartbeat interval (s) - 1 hours
}