# Phase 4 Pipeline Architecture

This document describes the end-to-end architecture of the Phase 4 multi-agent
telemetry pipeline for vehicle and robotics operational monitoring.

## Design Principle

**Rules decide → LLM explains**

All data processing, metric computation, anomaly detection, and health
classification are performed by deterministic C agents. LLMs are used
exclusively to explain decisions already made by rule-based logic.

## Pipeline Overview

The pipeline consists of seven agents executing in strict sequence:

```
CSV Data
  │
  ▼
┌─────────────────┐
│  Data Agent (C)  │  Parse CSV, validate rows, group by vehicle
└────────┬────────┘
         │ validated_data.json
         ▼
┌─────────────────────┐
│ Analysis Agent (C)  │  Compute KPIs, detect anomalies (3-sigma + domain)
└────────┬────────────┘
         │ analysis_results.json
         ▼
┌───────────────────────────┐
│ Classification Agent (C)  │  Assign Healthy/Warning/Critical per vehicle
└────────┬──────────────────┘
         │ fleet_status.json
         ▼
┌──────────────────────────┐
│ Search Agent (Sonnet)    │  Query RAG for relevant threshold/rule context
└────────┬─────────────────┘
         │ RAG context per vehicle
         ▼
┌───────────────────────────────┐
│ Explanation Agent (Opus)      │  Generate per-vehicle insights
└────────┬──────────────────────┘
         │ Insights per vehicle
         ▼
┌─────────────────────────┐
│ Formatting Agent (GPT)  │  Merge KPIs + status + insights into report
└────────┬────────────────┘
         │ final_report.txt
         ▼
┌──────────────────────────────┐
│ Coding Agent (GPT-4o-mini)   │  Validate report against CLAUDE.md
└────────┬─────────────────────┘
         │ compliance_report.txt
         ▼
      Done
```

## Agent Details

### C Deterministic Agents

These agents contain no LLM calls. They read JSON from stdin or file arguments
and write JSON to stdout.

| Agent | Model | Input | Output |
|-------|-------|-------|--------|
| data_agent | None (C) | CSV file path | validated_data.json |
| analysis_agent | None (C) | validated_data.json | analysis_results.json |
| classification_agent | None (C) | analysis_results.json | fleet_status.json |

### Python LLM Agents

Each agent uses a specific model and API. They are standalone functions
callable from main.py with no shared global state.

| Agent | Model | Provider | Role |
|-------|-------|----------|------|
| search_agent | Claude Sonnet | Anthropic | Query RAG for context |
| explanation_agent | Claude Opus | Anthropic | Generate vehicle insights |
| formatting_agent | GPT | OpenAI | Format final report |
| coding_agent | GPT-4o-mini | OpenAI | Validate compliance |

## RAG (Retrieval-Augmented Generation)

The RAG subsystem grounds LLM responses in project documentation:

- **Source documents** are stored in `docs/` as markdown files
- **rag_setup.py** chunks documents (~500 tokens, 50 overlap), embeds them
  using `sentence-transformers/all-MiniLM-L6-v2`, and persists the index
  to `vector_store/` using ChromaDB
- **rag_query.py** retrieves the top-k most relevant chunks for a given query
- The **search agent** uses RAG to find threshold definitions and classification
  rules before the explanation agent generates insights

## Inter-Agent Communication

- C agents communicate via **JSON over stdout/stdin**
- Python agents receive data as **Python dicts** (parsed from C agent JSON output)
- The orchestrator (`main.py`) manages the data flow between all agents

## Coding Guidelines

The project uses a three-layer coding guidelines system:

1. **Layer 1 (CLAUDE.md)** — Mandatory rules: separation of deterministic logic
   and LLM reasoning, strict interface contracts, memory safety
2. **Layer 2 (skills/SKILL.md)** — Self-check playbooks for writing C agents,
   Python agents, running the pipeline, and modifying documentation
3. **Layer 3 (Automated tooling)** — clang-format, pylint, pre-commit hooks,
   and CI workflows enforce style and quality automatically

## Key Metrics

The system monitors 10 telemetry metrics per vehicle:

1. Temperature (°C)
2. Latency (ms)
3. Error rate (ratio)
4. Battery voltage (V)
5. CPU usage (%)
6. Memory usage (%)
7. GPS accuracy (meters)
8. Network strength (dBm)
9. Vibration (g)
10. Wheel speed variance (km/h²)

Each metric has both a statistical threshold (3-sigma) and a domain-specific
threshold defined in the analysis agent.
