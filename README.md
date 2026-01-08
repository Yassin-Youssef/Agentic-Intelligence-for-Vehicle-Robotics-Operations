# Agentic Intelligence for Vehicle & Robotics Operations

This project implements an **agentic AI system** for monitoring, analyzing, and explaining  
vehicle and robotics operational metrics at scale.

The system is designed as a **progressive, multi-phase pipeline**, starting from fully  
deterministic analytics and evolving into **LLM-enhanced explainability**, while strictly  
preserving transparent, rule-based decision making.

The core architectural principle is:

**Rules decide → LLM explains**

---

## Architecture Overview

The system follows a consistent agentic pipeline across all phases:

**Sense → Decide → Analyze → Classify → Explain → Report**

Each stage is modular, testable, and independently extensible.

---

## Project Phases

The project is implemented in **three clearly defined phases**, each building on the previous one.

---

## Phase 1 — Single-Vehicle Deterministic Analysis

Phase 1 establishes the analytical foundation by processing telemetry for a **single vehicle**  
using deterministic logic only.

### Architecture

Sense → Analyze → Report

### Features

- CSV-based telemetry ingestion
- KPI computation:
  - Mean
  - Minimum
  - Maximum
  - Standard deviation
- Statistical anomaly detection (3-sigma rule)
- Rule-based anomaly detection using domain thresholds
- Fully deterministic and reproducible results
- Human-readable analytical report

### Outcome

Phase 1 demonstrates how raw telemetry can be transformed into structured diagnostics  
without heuristics, machine learning, or AI.

---

## Phase 2 — Fleet-Level Deterministic Agentic Pipeline

Phase 2 extends the system to support **fleet-level analysis**, where multiple vehicles are  
processed independently from a shared dataset.

### Architecture

Sense → Decide → Analyze → Report

### Components

- **PlannerAgent** — converts high-level objectives into analysis steps
- **CSV Loader** — ingests fleet telemetry
- **Metrics Analyzer** — computes KPIs and detects anomalies per vehicle
- **ReporterAgent** — generates structured fleet reports

### Features

- Vehicle-aware telemetry via `vehicle_id`
- Independent per-vehicle KPI computation
- Combined statistical and rule-based anomaly detection
- No cross-contamination between vehicles
- Fleet-level reporting with per-vehicle sections
- Support for healthy, warning, and failing vehicles
- Fully deterministic execution

### Outcome

Phase 2 proves scalability, modularity, and fleet awareness while maintaining  
full explainability and reproducibility.

---

## Phase 3 — LLM-Enhanced Explainability & Decision Support

Phase 3 introduces **LLM-based reasoning**, while preserving deterministic decision logic.

The LLM is **never** used to:
- Detect anomalies
- Compute KPIs
- Classify vehicle health

Instead, it **explains** decisions already made by deterministic rules.

### Architecture

Sense → Decide → Analyze → Classify → Explain (LLM) → Report

### New Components

- **Vehicle health classification**
- **LLM explanation module**
- **Enhanced reporting layer**

### Vehicle Health Classification

Each vehicle is deterministically classified as:

- **Healthy**
- **Warning**
- **Critical**

Based on:
- Total number of anomalies
- Number of affected metrics

### LLM-Based Explanations

For each vehicle, the system generates:

1. What is happening (concise summary)
2. Likely causes (grounded in observed metrics)
3. Recommended next actions (engineering-oriented)

### LLM Constraints

- No fabricated sensors or metrics
- No invented data
- Conservative, evidence-based explanations
- LLM output grounded strictly in computed analytics

### Outcome

Phase 3 transforms deterministic analytics into **human-readable, actionable intelligence**,  
making the system suitable for real-world engineering and operations workflows.

---

## Example Outputs

- Fleet-wide KPI computation
- Deterministic vehicle health classification
- Per-vehicle anomaly summaries
- LLM-generated operational insights

The final generated fleet report is saved to:

```
outputs/final_report.txt
```

---

## How to Run

```bash
python main.py
```

---

## Environment Configuration

Phase 3 uses an external LLM API for explanations.

Create a `.env` file based on `.env.example`:

```ini
OPENROUTER_API_KEY=your_api_key_here
OPENROUTER_MODEL=openai/gpt-4o-mini
```

The `.env` file is excluded from version control.

---

## Design Principles

- Deterministic core logic  
- Clear separation of concerns  
- LLMs used for explanation, not decision making  
- Modular and extensible architecture  
- Reproducible analytics  
- Industry-aligned engineering practices  

---

## Repository Structure

```
agents/     → Planning and reporting agents
tools/      → Deterministic analysis and LLM explanation tools
data/       → Sample fleet telemetry
docs/       → Report documentation
schemas/    → Report schema definitions
outputs/    → Generated reports
main.py     → Entry point
README.md   → Project documentation
```
