# Vehicle Health Classification Rules

This document defines the deterministic classification rules used by the
classification agent to assign a health status to each vehicle in the fleet.

## Overview

After the analysis agent computes KPIs and detects anomalies for each vehicle,
the classification agent evaluates two aggregate metrics:

1. **Total Anomalies** — The sum of all anomalous data points across all metrics
   for a single vehicle.
2. **Metrics Affected** — The count of distinct metrics that have at least one
   anomaly for a single vehicle.

These two values determine the vehicle's health status.

## Classification Rules

### Critical

A vehicle is classified as **Critical** if:
- `total_anomalies >= 20` **OR** `metrics_affected >= 7`

**Interpretation:** The vehicle has widespread or severe issues affecting
multiple systems. Immediate attention is required. The vehicle should be
taken out of service for inspection and repair.

**Example:** A vehicle with 44 total anomalies across 9 metrics would be
classified as Critical due to both conditions being met.

### Warning

A vehicle is classified as **Warning** if:
- `total_anomalies >= 5` **OR** `metrics_affected >= 3`
- AND the vehicle does not meet the Critical criteria

**Interpretation:** The vehicle shows moderate anomalies that warrant
monitoring and potential maintenance scheduling. The vehicle can continue
operating but should be prioritized for the next maintenance window.

**Example:** A vehicle with 8 total anomalies across 4 metrics would be
classified as Warning.

### Healthy

A vehicle is classified as **Healthy** if:
- It does not meet the Warning or Critical criteria
- Effectively: `total_anomalies < 5` **AND** `metrics_affected < 3`

**Interpretation:** The vehicle is operating within normal parameters.
No immediate action is required. Continue routine monitoring.

**Example:** A vehicle with 2 total anomalies across 1 metric would be
classified as Healthy.

## Priority Order

The classification is evaluated in priority order:
1. Check Critical conditions first
2. If not Critical, check Warning conditions
3. If neither, assign Healthy

This ensures that a vehicle meeting Critical thresholds is never
downgraded to Warning, even if it also meets Warning conditions.

## Anomaly Counting Details

- **Total anomalies** counts individual anomalous data points, not unique
  anomaly types. A metric with 5 anomalous readings contributes 5 to the total.
- **Metrics affected** counts how many distinct metric names (e.g., temperature,
  latency) have at least one anomaly. A metric with zero anomalies is not counted.
- Both statistical anomalies (3-sigma) and rule-based anomalies (domain thresholds)
  are included in the counts after deduplication.

## Relationship to Other Components

- The classification agent receives its input from the **analysis agent**, which
  provides per-vehicle KPIs and anomaly lists.
- The classification output is consumed by the **search agent** and
  **explanation agent** to generate human-readable insights.
- Classification is **fully deterministic** — the same input always produces
  the same classification. No LLM or probabilistic logic is involved.
