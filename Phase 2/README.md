# Phase 2 — Fleet-Level Deterministic Agentic Pipeline

This phase extends the deterministic agentic system to support  
fleet-level analysis of vehicle and robotics operational metrics.

Instead of analyzing a single system in isolation, Phase 2 introduces  
vehicle-aware processing, where multiple vehicles are analyzed  
independently from a shared telemetry dataset.

## Architecture

Sense → Decide → Analyze → Report

## Components

- PlannerAgent (Decide)
- CSV Loader (Sense)
- Metrics Analyzer (Analyze)
- ReporterAgent (Report)

## Features

- Vehicle-aware telemetry using vehicle_id
- Per-vehicle KPI computation (mean, min, max, std)
- Statistical anomaly detection (3-sigma rule)
- Rule-based anomaly detection using domain thresholds
- Independent analysis for each vehicle in the fleet
- Human-readable fleet report with per-vehicle sections
- Support for vehicles with no anomalies, few anomalies, or many anomalies

## How to Run

```bash
python main.py
