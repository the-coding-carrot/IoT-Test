# IoT Mailbox Monitor

An ESP32-based smart mailbox system that detects mail delivery using an HC-SR04 ultrasonic distance sensor.

## Overview

The system monitors the distance from the top of a mailbox to the floor. When mail is delivered, the distance decreases, triggering a detection event. Multiple layers of filtering and validation prevent false positives from insects, vibrations, or sensor noise.

## How It Works

1. **Ultrasonic sensor** measures distance to mailbox floor every second
2. **Median filter** smooths readings and removes outliers
3. **Detection algorithm** identifies persistent distance changes (mail delivery)
4. **Telemetry system** logs events and periodic status updates

See the [Detection Logic Explanation](docs/detection_logic.md) for detailed information.

## Hardware Requirements

- ESP32 development board
- HC-SR04 ultrasonic distance sensor
- LED (onboard or external)
- Power supply (USB or battery)

## Project Structure

```
├── config/
│   └── config.hpp                    # Global configuration constants
│
├── hardware/
│   ├── led/
│   │   ├── led.hpp                   # LED control interface
│   │   └── led.cpp                   # LED implementation
│   │
│   └── ultrasonic/
│       ├── hcsr04.hpp                # HC-SR04 sensor interface
│       └── hcsr04.cpp                # HC-SR04 sensor implementation
│
├── processor/
│   └── distance/
│       ├── distance_processor.hpp    # Distance processing & detection
│       └── distance_processor.cpp    # Filtering, tracking, state machine
│
├── telemetry/
│   └── distance/
│       ├── distance_telemetry.hpp    # Telemetry publishing interface
│       └── distance_telemetry.cpp    # JSON formatting & logging
│
└── main.cpp                          # Application entry point
```

## Architecture

The system follows a simple three-layer pipeline:

```
HCSR04 Sensor → Distance Processor → Telemetry Publisher
    (raw)           (filtered)           (formatted)
```

### Layer Responsibilities

| Layer                 | Purpose                                | Outputs                                         |
| --------------------- | -------------------------------------- | ----------------------------------------------- |
| **HCSR04**            | Raw distance measurement               | Distance in cm (or -1 on timeout)               |
| **DistanceProcessor** | Filtering, detection, quality tracking | Structured `DistanceData` with detection status |
| **DistanceTelemetry** | JSON formatting and publishing         | Event logs and periodic status updates          |

### Data Flow

```cpp
// main.cpp - Simple and clear
while (true) {
    // 1. Measure
    float raw = ultrasonic.MeasureDistance();

    // 2. Process (filter, detect, track)
    DistanceData data = processor.Process(raw);

    // 3. Publish telemetry
    telemetry.Publish(data, processor.GetBaseline(), processor.GetThreshold());

    // 4. Wait for next cycle
    vTaskDelay(pdMS_TO_TICKS(1000));
}
```

## Configuration

All system parameters are defined in `config/config.hpp`:

### Key Parameters

```cpp
// Hardware pins
HCSR04_TRIGGER_PIN = GPIO_NUM_2
HCSR04_ECHO_PIN = GPIO_NUM_3
LED_PIN = GPIO_NUM_8

// Detection sensitivity
BASELINE_CM = 40.0          // Empty mailbox distance (CALIBRATE THIS!)
TRIGGER_DELTA_CM = 3.0      // Minimum change to detect (cm)
HOLD_MS = 250               // Must persist this long (ms)
REFRACTORY_MS = 8000        // Cooldown between events (ms)

// Signal processing
FILTER_WINDOW = 5           // Median filter size (samples)
DISTANCE_MEASUREMENT_INTERVAL_MS = 1000  // Measurement frequency
```

### Calibration

1. **Measure your mailbox**: Place sensor at top, measure distance to empty floor
2. **Set `BASELINE_CM`**: Update in `config.hpp` with your measurement
3. **Adjust sensitivity**: Tune `TRIGGER_DELTA_CM` based on typical mail thickness
4. **Test**: Monitor logs and adjust `HOLD_MS` if getting false positives

## Telemetry Output

### Periodic Status (every 2 seconds)

```json
{
  "telemetry": true,
  "distance_cm": 39.8,
  "filtered_cm": 40.1,
  "baseline_cm": 40.0,
  "threshold_cm": 37.0,
  "success_rate": 0.98
}
```

### Mail Drop Event (when detected)

```json
{
  "event": "mail_drop",
  "baseline_cm": 40.0,
  "before_cm": 40.0,
  "after_cm": 37.2,
  "delta_cm": 2.8,
  "duration_ms": 485,
  "confidence": 0.87,
  "success_rate": 0.98
}
```
