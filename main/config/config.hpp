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
    // Ultrasonic Sensor (HC-SR04)
    // ──────────────────────────────
    constexpr gpio_num_t HCSR04_TRIGGER_PIN = GPIO_NUM_5;       // Trigger pin (output)
    constexpr gpio_num_t HCSR04_ECHO_PIN = GPIO_NUM_18;         // Echo pin (input)
    constexpr uint32_t DISTANCE_MEASUREMENT_INTERVAL_MS = 1000; // Measurement interval (ms)
    constexpr uint32_t TRIGGER_PULSE_uS = 10;                   // Trigger pulse duration (µs)
    constexpr float DISTANCE_THRESHOLD_CM = 400.0f;             // Max valid distance (cm)
    constexpr uint32_t ECHO_TIMEOUT_US = 35000;                 // Echo timeout (µs)

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
    static constexpr const char *MQTT_BROKER_URI = "mqtt://10.178.116.70:1883"; // Broker URI
    static constexpr const char *MQTT_BASE_TOPIC = "home/mailbox";              // Base topic
    static constexpr const char *MQTT_CLIENT_ID = "mailbox-sensor-001";         // Client ID

    // ──────────────────────────────
    // Wi-Fi Settings
    // ──────────────────────────────
    static constexpr const char *CONN_SSID = "teaofthehe"; // Wi-Fi SSID
    static constexpr const char *PASSWORD = "11235813";    // Wi-Fi password

    // ──────────────────────────────
    // Power Management
    // ──────────────────────────────
    static constexpr uint64_t DEEP_SLEEP_US = 5000000;       // Deep sleep duration (µs) - 5 seconds
    static constexpr uint64_t HEARTBEAT_INTERVAL_SEC = 3600; // Heartbeat interval (s) - 1 hours
}