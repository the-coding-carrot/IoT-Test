# Mail Drop Detection Logic

## Overview

The system uses an ultrasonic distance sensor (HC-SR04) mounted at the top of a mailbox, pointing downward at the mailbox floor. It detects mail delivery by recognizing when the measured distance **suddenly decreases** and **stays decreased** for a specific duration. The system uses a **state machine** to track mailbox status and prevent false duplicate events.

---

## The Basic Principle

### Empty Mailbox (Baseline)

```
Sensor (top of mailbox)
    |
    | 40 cm (BASELINE_CM)
    ↓
Floor (empty)
```

- **Baseline Distance**: 40 cm from sensor to empty mailbox floor
- This is your calibrated "normal" state
- **Mailbox State**: EMPTY

### Mail Delivered

```
Sensor (top of mailbox)
    |
    | 37 cm (reduced distance)
    ↓
Mail (on top of floor)
    |
Floor
```

- Mail creates an **occlusion** - something is now closer to the sensor
- New distance: ~37 cm (3 cm closer than baseline)
- **Mailbox State**: HAS_MAIL

### Full Mailbox

```
Sensor (top of mailbox)
    |
    | 34 cm (significantly reduced)
    ↓
Multiple letters/packages
    |
Floor
```

- Multiple items stack up
- Distance drops below full threshold (baseline - 2×delta = 34 cm)
- **Mailbox State**: FULL

---

## Mailbox State Machine

The system tracks four distinct states to prevent false events:

### 🟢 **EMPTY** - Ready for Mail

- Distance ≈ baseline (40 cm)
- No mail present
- Waiting for delivery
- **Can trigger**: `mail_drop` event

### 🟡 **HAS_MAIL** - Mail Present

- Distance below trigger threshold (< 37 cm)
- Mail detected and confirmed
- **Cannot trigger**: More `mail_drop` events (prevents duplicates!)
- **Can trigger**: `mail_collected` event
- **Can transition to**: FULL state

### 🔴 **FULL** - Mailbox Full

- Distance significantly reduced (< 34 cm)
- Multiple items present
- **Cannot trigger**: `mail_drop` events
- **Can trigger**: `mail_collected` event

### 🔵 **EMPTIED** - Just Collected (Transitional)

- Distance returned to near baseline
- Mail was just removed
- Brief state (~250 ms) before returning to EMPTY
- Prevents immediate re-triggering

---

## State Transitions

```
     ┌─────────────────────────────────────┐
     │         EMPTY (Ready)                │
     │   Distance ≈ 40 cm                   │
     └──────────────┬──────────────────────┘
                    │
         New mail detected (< 37 cm, held 250ms)
         📬 EVENT: "mail_drop"
                    │
                    ↓
     ┌──────────────────────────────────────┐
     │       HAS_MAIL (Mail Present)        │
     │   Distance < 37 cm                   │
     └──────┬────────────────────┬──────────┘
            │                    │
    More mail arrives    Mail collected (> 38.5 cm)
    (< 34 cm)           🗑️ EVENT: "mail_collected"
            │                    │
            ↓                    ↓
     ┌──────────────┐   ┌──────────────────┐
     │     FULL     │   │    EMPTIED       │
     │  (Overload)  │   │  (Transitional)  │
     └──────┬───────┘   └────────┬─────────┘
            │                    │
    Mail collected         Wait 250ms
    (> 38.5 cm)                 │
    🗑️ EVENT: "mail_collected"  │
            │                    │
            └────────────────────┘
                    │
                    ↓
              Back to EMPTY
```

### Key Insight: **One Mail Drop = One Event**

- Once mail is detected, state changes to HAS_MAIL or FULL
- **No more `mail_drop` events** until mailbox is emptied
- This solves the problem of repeated false positives from mail sitting in the box

---

## Detection Pipeline Stages

### 1. **Idle Monitoring** (EMPTY State)

- Filtered distance ≈ baseline (40 cm)
- System continuously monitors for occlusions
- Ready to detect new mail

### 2. **Occlusion Detected** (Potential Mail)

- Filtered distance drops below **trigger threshold**
  - Empty threshold = `BASELINE_CM - (TRIGGER_DELTA_CM × 0.5)`
  - Empty threshold = 40 cm - 1.5 cm = **38.5 cm**
  - Trigger threshold = `BASELINE_CM - TRIGGER_DELTA_CM`
  - Trigger threshold = 40 cm - 3 cm = **37 cm**
- Timer starts: Record `occlusion_start_us_`
- Set `occluding_ = true`

### 3. **Debounce Period** (Filtering False Positives)

- Occlusion must **persist** for at least `HOLD_MS` (250 ms)
- System checks: Has the distance stayed below threshold long enough?
- **Purpose**: Ignore brief disturbances

### 4. **Event Triggered** (Mail Confirmed)

- If occlusion held ≥ 250 ms → **Mail drop event confirmed!**
- **State transition**: EMPTY → HAS_MAIL
- Log event with:
  - Delta: How much closer (cm)
  - Duration: How long it was held (ms)
  - Confidence: Quality score
  - New state: HAS_MAIL
- Enter **refractory period**

### 5. **Mail Present State** (HAS_MAIL or FULL)

- System monitors but **does not trigger new `mail_drop` events**
- If distance drops further (< 34 cm) → transition to FULL state
- Waits for collection (distance returns to > 38.5 cm)

### 6. **Collection Detection**

- Distance rises above empty threshold (38.5 cm)
- Must persist for 250 ms
- **Event triggered**: `mail_collected`
- **State transition**: HAS_MAIL/FULL → EMPTIED

### 7. **Refractory Period** (Cooldown)

- Duration: `REFRACTORY_MS` (8000 ms = 8 seconds)
- Applied after both mail detection AND collection
- Prevents rapid re-triggering
- After cooldown + 250ms hold, returns to EMPTY state

---

## False Positive Prevention

The system uses **multiple layers of protection**:

### 🔹 Layer 1: Median Filter (`FILTER_WINDOW = 5`)

**Problem**: Sensor readings can be noisy or have occasional outliers

**Solution**: Takes 5 consecutive measurements and uses the **median** (middle value)

- Single bad reading won't affect the filtered result
- Smooth out sensor jitter
- More reliable than raw measurements

**Example**:

```
Raw readings: [40.2, 39.8, 15.0 (outlier!), 40.1, 39.9]
Sorted:       [15.0, 39.8, 39.9, 40.1, 40.2]
Median:       39.9 cm ✓ (outlier ignored)
```

### 🔹 Layer 2: Multiple Thresholds

**Problem**: Need to distinguish between empty, has mail, and full states

**Solution**: Three calibrated thresholds:

| Threshold | Formula                  | Value   | Purpose                             |
| --------- | ------------------------ | ------- | ----------------------------------- |
| Empty     | Baseline - (Delta × 0.5) | 38.5 cm | Detect when mailbox is empty again  |
| Trigger   | Baseline - Delta         | 37 cm   | Detect new mail arrival             |
| Full      | Baseline - (Delta × 2)   | 34 cm   | Detect when mailbox is getting full |

### 🔹 Layer 3: State Machine

**Problem**: Mail sitting in box would repeatedly trigger events

**Solution**: Track mailbox state - only trigger `mail_drop` from EMPTY state

- **EMPTY** → Can detect new mail ✓
- **HAS_MAIL** → Already has mail, ignore further occlusions ✗
- **FULL** → Already full, ignore occlusions ✗
- **EMPTIED** → Transitional state, brief cooldown

### 🔹 Layer 4: Hold Time (`HOLD_MS = 250`)

**Problem**: Brief objects passing by (insects, shadows, hand reaching in)

**Solution**: Occlusion must **persist continuously** for 250 milliseconds

- Transient disturbances last < 100 ms
- Mail stays in place after delivery
- 250 ms = reasonable compromise between speed and reliability

**Timeline example**:

```
Time:    0ms    50ms   100ms  150ms  200ms  250ms  ✓ EVENT
         ↓      ↓      ↓      ↓      ↓      ↓
Distance: 37cm → 37cm → 37cm → 37cm → 37cm → 37cm (held!)

vs. False positive:
Time:    0ms    50ms   100ms
         ↓      ↓      ↓
Distance: 37cm → 37cm → 40cm (returned to baseline, ignored)
```

### 🔹 Layer 5: Refractory Period (`REFRACTORY_MS = 8000`)

**Problem**: Mail might shift/settle after delivery or collection

**Solution**: After any event, **ignore all new triggers for 8 seconds**

- Applied after both `mail_drop` and `mail_collected` events
- Prevents alert spam
- Gives time for mail to fully settle

### 🔹 Layer 6: Confidence Score

**Problem**: Some detections are more reliable than others

**Solution**: Calculate a **confidence score** (0.0 to 1.0) based on:

```
Confidence = 0.5 × (distance_change / 3cm)      [50% weight]
           + 0.3 × (held_duration / 250ms)      [30% weight]
           + 0.2 × (sensor_success_rate)        [20% weight]
```

**High confidence example** (0.9+):

- Mail is 4 cm thick (distance_change = 4 cm) → Factor: 4/3 = 1.33
- Held for 500 ms (duration = 500 ms) → Factor: 500/250 = 2.0
- Sensor working perfectly (success_rate = 1.0) → Factor: 1.0
- **Confidence** = min(1.0, 0.5×1.33 + 0.3×2.0 + 0.2×1.0) = **1.0**

**Low confidence example** (0.5):

- Barely triggers (distance_change = 3 cm) → Factor: 3/3 = 1.0
- Just meets hold time (duration = 250 ms) → Factor: 250/250 = 1.0
- Sensor having issues (success_rate = 0.5) → Factor: 0.5
- **Confidence** = 0.5×1.0 + 0.3×1.0 + 0.2×0.5 = **0.6**

---

## Common Scenarios

### ✅ **True Positive: Mail Delivered**

```
Time:     0s    1s    2s    3s    4s
Distance: 40cm→ 37cm→ 37cm→ 37cm→ 37cm
Status:   EMPTY→ Occl→ Occl→ HAS_MAIL→ HAS_MAIL
Event:    -      -      -    📬 mail_drop  -
```

- Distance drops 3+ cm ✓
- Stays below threshold ✓
- Held for 250+ ms ✓
- **MAIL DETECTED** 📬
- State: EMPTY → HAS_MAIL

### ✅ **No Duplicate Events: Mail Sitting in Box**

```
Time:     10s   20s   30s   40s   50s
Distance: 37cm→ 37cm→ 37cm→ 37cm→ 37cm
Status:   HAS_MAIL (stable)
Event:    -      -      -      -      -
```

- Mail continues to sit in box
- Distance remains below trigger threshold
- **NO NEW EVENTS** ✓ (state is HAS_MAIL, not EMPTY)
- This is the key fix - prevents false duplicates!

### ✅ **True Positive: Mail Collected**

```
Time:     60s   61s   62s   63s   64s   65s
Distance: 37cm→ 39cm→ 39cm→ 40cm→ 40cm→ 40cm
Status:   HAS_MAIL→ Occl→ Occl→ EMPTIED→ EMPTY
Event:    -       -      -    🗑️ collected  -
```

- Distance rises back to baseline ✓
- Held above empty threshold for 250+ ms ✓
- **MAIL COLLECTED** 🗑️
- State: HAS_MAIL → EMPTIED → EMPTY

### ✅ **True Positive: Multiple Deliveries (Separate Events)**

```
Event 1: 10:00:00 - Mail delivered
         State: EMPTY → HAS_MAIL
         Event: 📬 mail_drop

         Mail sits in box (10:00:01 to 10:05:00)
         State: HAS_MAIL (no new events)

Event 2: 10:05:00 - Mail collected
         State: HAS_MAIL → EMPTIED → EMPTY
         Event: 🗑️ mail_collected

Event 3: 10:06:00 - New mail delivered
         State: EMPTY → HAS_MAIL
         Event: 📬 mail_drop (new event, correctly triggered!)
```

### ✅ **True Positive: Mailbox Getting Full**

```
Time:     0s      5s       10s
Distance: 40cm → 37cm  →  34cm
Status:   EMPTY→ HAS_MAIL→ FULL
Event:    -      📬 drop   -
```

- First mail: Triggers event, enters HAS_MAIL
- More mail arrives: Transitions to FULL (no new event)
- Only one `mail_drop` event for the entire filling process ✓

### ❌ **False Positive: Hand Reaching In**

```
Time:     0s    0.1s  0.2s  0.3s
Distance: 40cm→ 20cm→ 20cm→ 40cm (hand withdrawn)
Status:   EMPTY→ Occl→ Occl→ EMPTY (canceled)
Event:    -      -      -      -
```

- Distance drops significantly ✓
- But only held for 200 ms ✗
- Returns to baseline before HOLD_MS expires
- **NO EVENT** (correctly ignored)

### ❌ **False Positive: Insect/Spider**

```
Time:     0s    0.05s 0.1s
Distance: 40cm→ 35cm→ 40cm
Status:   EMPTY→ Brief→ EMPTY
Event:    -      -      -
```

- Brief occlusion < 50 ms
- Filtered out by median filter
- Never triggers occlusion state
- **NO EVENT** (correctly ignored)

### ❌ **False Positive: Sensor Glitch**

```
Raw:      40cm, 40cm, 2cm (glitch!), 40cm, 40cm
Filtered: 40cm (median ignores the 2cm outlier)
Status:   EMPTY (never triggers)
Event:    -
```

- Single bad reading
- Median filter removes it
- **NO EVENT** (correctly ignored)

---

## Telemetry Events

The system publishes three types of telemetry:

### 📬 **Mail Drop Event** (Immediate)

```json
{
  "event": "mail_drop",
  "baseline_cm": 40.0,
  "before_cm": 40.0,
  "after_cm": 37.2,
  "delta_cm": 2.8,
  "duration_ms": 350,
  "confidence": 0.85,
  "success_rate": 0.97,
  "new_state": "has_mail"
}
```

- Published immediately when mail is detected
- Only fires from EMPTY state
- Includes confidence score and state change

### 🗑️ **Mail Collected Event** (Immediate)

```json
{
  "event": "mail_collected",
  "baseline_cm": 40.0,
  "before_cm": 37.2,
  "after_cm": 39.8,
  "delta_cm": 2.6,
  "duration_ms": 280,
  "success_rate": 0.96,
  "new_state": "emptied"
}
```

- Published when mailbox is emptied
- Fires from HAS_MAIL or FULL states
- Indicates mailbox is being emptied

### 📊 **Periodic Status Telemetry** (Every 10 seconds)

```json
{
  "telemetry": true,
  "distance_cm": 37.1,
  "filtered_cm": 37.2,
  "baseline_cm": 40.0,
  "threshold_cm": 37.0,
  "success_rate": 0.97,
  "mailbox_state": "has_mail"
}
```

- Published at regular intervals (TELEMETRY_PERIOD_MS)
- Includes current mailbox state
- Useful for monitoring and dashboards
- States: `"empty"`, `"has_mail"`, `"full"`, `"emptied"`

---

## Success Rate Tracking

The system also tracks **measurement reliability**:

```cpp
success_rate = successful_readings / total_readings
```

- **Good sensor**: success_rate ≈ 0.95-1.0
- **Problems** (dust, alignment, power): success_rate < 0.8
- Used in confidence calculation
- Automatically decays old history every ~60 seconds

---

## Configuration Tuning

You can adjust these parameters in `config.hpp`:

| Parameter          | Default | Purpose                | Tuning Guide                                       |
| ------------------ | ------- | ---------------------- | -------------------------------------------------- |
| `BASELINE_CM`      | 40 cm   | Empty mailbox distance | **Measure your mailbox!**                          |
| `TRIGGER_DELTA_CM` | 3 cm    | Sensitivity threshold  | ↑ = less sensitive, ↓ = more sensitive             |
| `HOLD_MS`          | 250 ms  | Debounce duration      | ↑ = fewer false positives, slower response         |
| `REFRACTORY_MS`    | 8000 ms | Cooldown period        | ↑ = prevents duplicates, may miss rapid deliveries |
| `FILTER_WINDOW`    | 5       | Median filter size     | ↑ = smoother but slower, ↓ = faster but noisier    |

**Derived thresholds** (automatically calculated):

- Empty threshold = `BASELINE_CM - (TRIGGER_DELTA_CM × 0.5)` = 38.5 cm
- Trigger threshold = `BASELINE_CM - TRIGGER_DELTA_CM` = 37 cm
- Full threshold = `BASELINE_CM - (TRIGGER_DELTA_CM × 2)` = 34 cm

---

## Visual Summary

```
📊 Detection Pipeline with State Machine:

Raw Sensor → Median Filter → State Machine → Event Decision
   ↓              ↓               ↓              ↓
  40cm          40cm         EMPTY          (waiting)
  37cm          37cm         EMPTY          📬 mail_drop!
  37cm          37cm         HAS_MAIL       (no event)
  37cm          37cm         HAS_MAIL       (no event)
  39cm          39cm         HAS_MAIL       (waiting)
  40cm          40cm         EMPTIED        🗑️ collected!
  40cm          40cm         EMPTY          (ready)

  ↑ Noise       ↑ Smooth     ↑ Track State  ↑ Prevent Duplicates
```

### State-Based Event Logic

```
┌─────────────────────────────────────────┐
│  EMPTY State (Distance ≈ 40cm)          │
│  ✅ Can trigger: mail_drop              │
│  ❌ Cannot trigger: mail_collected      │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  HAS_MAIL State (Distance < 37cm)       │
│  ❌ Cannot trigger: mail_drop           │
│  ✅ Can trigger: mail_collected         │
└─────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────┐
│  FULL State (Distance < 34cm)           │
│  ❌ Cannot trigger: mail_drop           │
│  ✅ Can trigger: mail_collected         │
└─────────────────────────────────────────┘
```

## Key Insights

1. **Mail is persistent, false positives are transient** - By requiring sustained occlusions, we filter out momentary disturbances

2. **State machine prevents duplicate events** - Once mail is detected, the system remembers and won't trigger again until the mailbox is emptied

3. **Multiple thresholds enable smart transitions** - Different thresholds for empty detection, mail detection, and full detection

4. **Both delivery and collection are tracked** - System provides complete lifecycle visibility of mailbox contents

5. **Layered defense strategy** - Multiple independent filtering layers work together to ensure reliability
