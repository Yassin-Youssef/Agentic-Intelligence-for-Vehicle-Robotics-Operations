# Domain Thresholds — Vehicle Telemetry Anomaly Detection

This document defines all domain-specific thresholds used by the analysis agent
to detect rule-based anomalies in vehicle telemetry data. These thresholds are
applied independently per vehicle, per metric, on every data point.

## How Thresholds Work

For each telemetry reading, the analysis agent checks whether the value exceeds
the domain-specific threshold for that metric. If it does, the reading is flagged
as a **rule-based anomaly**. Rule-based anomalies are combined with statistical
anomalies (3-sigma rule) to produce the final anomaly list per metric.

## Threshold Definitions

### Temperature
- **Condition:** temperature > 120 °C
- **Meaning:** Overheating detected. The vehicle's thermal management system
  may be failing, coolant levels may be low, or the vehicle is operating under
  excessive load.
- **Normal range:** 60–120 °C

### Latency
- **Condition:** latency > 800 ms
- **Meaning:** Excessive communication delay between the vehicle's control
  systems. May indicate network congestion, failing communication hardware,
  or software processing bottlenecks.
- **Normal range:** 0–800 ms

### Error Rate
- **Condition:** error_rate > 0.3 (30%)
- **Meaning:** Error rate beyond acceptable operational limits. Indicates
  significant system instability, potential sensor failures, or software bugs.
- **Normal range:** 0–0.3 (0–30%)

### Battery Voltage
- **Condition:** battery_voltage < 11.5 V OR battery_voltage > 13.0 V
- **Meaning:** Voltage outside the safe operating range. Low voltage may
  indicate a failing battery or excessive power draw. High voltage may indicate
  a charging system malfunction.
- **Normal range:** 11.5–13.0 V

### CPU Usage
- **Condition:** cpu_usage > 90%
- **Meaning:** CPU saturation. The vehicle's onboard computer is at or near
  full capacity, which may cause delayed response times and missed real-time
  deadlines.
- **Normal range:** 0–90%

### Memory Usage
- **Condition:** memory_usage > 85%
- **Meaning:** Memory pressure. The system is running low on available memory,
  which may cause out-of-memory errors, swapping, or degraded performance.
- **Normal range:** 0–85%

### GPS Accuracy
- **Condition:** gps_accuracy > 5.0 meters
- **Meaning:** Poor localization accuracy. The vehicle's GPS system is providing
  inaccurate position data, which may affect navigation, autonomous driving,
  or geofencing operations.
- **Normal range:** 0–5.0 meters

### Network Strength
- **Condition:** network_strength <= -90 dBm
- **Meaning:** Weak signal strength. The vehicle's cellular or wireless
  connection is degraded, which may cause communication drops, delayed
  telemetry uploads, or loss of remote control capability.
- **Normal range:** Greater than -90 dBm (closer to 0 is stronger)

### Vibration
- **Condition:** vibration > 2.0 g
- **Meaning:** Excessive mechanical vibration indicating potential wear,
  loose components, unbalanced wheels, or road surface issues.
- **Normal range:** 0–2.0 g

### Wheel Speed Variance
- **Condition:** wheel_speed_variance > 15 km/h²
- **Meaning:** High variance in wheel speeds may indicate traction control
  issues, wheel slip, differential problems, or driving on uneven surfaces.
- **Normal range:** 0–15 km/h²

## Statistical Anomaly Detection (3-Sigma Rule)

In addition to domain thresholds, the analysis agent applies the **3-sigma rule**
to each metric independently per vehicle:

- A value is a statistical anomaly if it falls outside the range:
  `mean ± 3 × standard_deviation`
- This detects outliers relative to the vehicle's own data distribution.
- The standard deviation uses **sample std (ddof=1)**, matching pandas default.
- If the standard deviation is zero (all values identical), no statistical
  anomalies are detected.

## Anomaly Combination

The final anomaly list for each metric is the **union** of:
1. Statistical anomalies (3-sigma violations)
2. Rule-based anomalies (domain threshold violations)

Duplicate values are removed. The combined list is used for classification.
