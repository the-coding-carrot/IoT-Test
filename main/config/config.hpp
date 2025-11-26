#pragma once

#include "driver/gpio.h"

namespace Config
{
    // ──────────────────────────────
    // Application Info
    // ──────────────────────────────
    constexpr const char *APP_NAME = "IoT Test"; // Application name
    constexpr const char *APP_VERSION = "1.0.0"; // Application version

    // ──────────────────────────────
    // LED Settings
    // ──────────────────────────────
    constexpr gpio_num_t LED_PIN = GPIO_NUM_2;  // Built-in LED on many ESP32-WROOM-32 boards
    constexpr bool LED_ACTIVE_LOW = false;      // ESP32-WROOM-32 typically active high
    constexpr uint32_t STARTUP_BLINK_COUNT = 5; // Number of startup blinks
    constexpr uint32_t STARTUP_BLINK_MS = 1000; // Blink duration (ms)

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
    constexpr float BASELINE_CM = 40.0f;     // Empty mailbox baseline (cm)
    constexpr float TRIGGER_DELTA_CM = 3.0f; // Min change to detect occlusion (cm)
    constexpr uint32_t HOLD_MS = 250;        // Occlusion hold time (ms)
    constexpr uint32_t REFRACTORY_MS = 8000; // Refractory period after detection (ms)

    // ──────────────────────────────
    // Filtering
    // ──────────────────────────────
    constexpr uint8_t FILTER_WINDOW = 5; // Median filter window size

    // ──────────────────────────────
    // MQTT Settings
    // ──────────────────────────────
    constexpr const char *MQTT_BROKER_URI = "mqtt://192.168.1.100:1883"; // Broker URI
    constexpr const char *MQTT_BASE_TOPIC = "home/mailbox";              // Base topic
    constexpr const char *MQTT_CLIENT_ID = "mailbox-sensor-001";         // Client ID

    // ──────────────────────────────
    // Wi-Fi Settings
    // ──────────────────────────────
    constexpr const char *CONN_SSID = "Test"; // Wi-Fi SSID
    constexpr const char *PASSWORD = "Test";  // Wi-Fi password

    // ──────────────────────────────
    // Power Management
    // ──────────────────────────────
    constexpr uint64_t DEEP_SLEEP_US = 10000000;      // Deep sleep duration (µs) - 10 seconds
    constexpr uint64_t HEARTBEAT_INTERVAL_SEC = 7200; // Heartbeat interval (s) - 2 hours
}