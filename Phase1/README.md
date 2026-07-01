# Phase 1 — Deterministic Agentic Pipeline

This phase implements a deterministic agentic system for analyzing
vehicle and robotics operational metrics.

## Architecture
Sense → Decide → Analyze → Report

## Components
- PlannerAgent (Decide)
- CSV Loader (Sense)
- Metrics Analyzer (Analyze)
- ReporterAgent (Report)

## Features
- KPI computation (mean, min, max, std)
- Statistical anomaly detection (3-sigma)
- Rule-based anomaly detection
- Human-readable reporting

## How to Run
```bash
python main.py
