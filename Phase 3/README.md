# Phase 3 — LLM-Enhanced Explainability Layer

This phase extends the deterministic fleet-level agentic pipeline by adding  
an LLM-based explainability layer on top of the existing analytics.

Instead of only reporting numerical KPIs and anomaly counts, Phase 3 introduces  
human-readable technical explanations that interpret vehicle health,  
anomaly patterns, and operational risks.

## Architecture

Sense → Decide → Analyze → Classify → LLM Explain → Report

## Components

- PlannerAgent (Decide)
- CSV Loader (Sense)
- Metrics Analyzer (Analyze)
- Vehicle Classifier (Classify)
- LLM Explainer (LLM Explain)
- ReporterAgent (Report)

## Features

- Deterministic vehicle health classification (Healthy / Warning / Critical)
- Fleet-level KPI computation and anomaly detection (inherited from Phase 2)
- Rule-based anomaly aggregation per vehicle
- LLM-generated natural-language explanations per vehicle
- AI insights structured as:
  - What is happening
  - Likely causes
  - Recommended next actions
- Clear separation between rule-based decisions and AI reasoning
- Human-readable operational report enriched with LLM explanations

## How to Run

```bash
python main.py
