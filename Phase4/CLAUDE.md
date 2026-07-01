# CLAUDE.md — Phase 4 Coding Guidelines

> **Layer 1: Law Book** — Mandatory rules for all agents (human and AI).
> Every contributor and every AI agent operating on this codebase must follow these rules.
> Violations are caught by Layer 3 (automated tooling) and flagged by the Coding Agent (GPT-4o-mini).

---

## Coding Guideline 1 — Separation of Deterministic Logic and LLM Reasoning

**All data processing, metric computation, anomaly detection, and health classification
must be implemented in deterministic, reproducible code. LLMs are never used to make
decisions — they only explain decisions already made by rule-based logic.**

### Rules

1. **C agents own all decisions.** `data_agent.c`, `analysis_agent.c`, and
   `classification_agent.c` contain the only code that loads data, computes KPIs,
   detects anomalies, and classifies vehicles. No Python agent may re-implement
   or override this logic.

2. **Python agents own all explanations.** `search_agent.py`, `explanation_agent.py`,
   `formatting_agent.py`, and `coding_agent.py` consume the JSON outputs of C agents
   and produce human-readable text. They never compute KPIs, apply thresholds, or
   change classification results.

3. **No LLM in the critical path.** If the LLM API is unavailable, the C pipeline
   must still produce correct `validated_data.json`, `analysis_results.json`, and
   `fleet_status.json`. Python agents degrade gracefully (skip or use mock mode).

4. **Thresholds live in C only.** Domain thresholds (temperature > 120, latency > 800,
   error_rate > 0.3, etc.) are defined exclusively in `analysis_agent.c`. They are
   documented in `docs/thresholds.md` for RAG retrieval but never evaluated in Python.

5. **Classification rules live in C only.** The Healthy/Warning/Critical boundaries
   (total_anomalies ≥ 20 or metrics_affected ≥ 7 → Critical, etc.) are defined
   exclusively in `classification_agent.c` and documented in `docs/classification_rules.md`.

### How to verify

- Grep Python agents for arithmetic on metric values → must find zero instances.
- Run the C pipeline alone without API keys → must produce valid JSON outputs.
- Compare C agent output with Phase 3 Python output on the same CSV → must match exactly.

---

## Coding Guideline 2 — Strict Interface Contracts and Memory Safety

**Every agent communicates through well-defined JSON schemas over stdin/stdout (C) or
function signatures (Python). C code must be memory-safe with no leaks, no undefined
behavior, and no unchecked allocations.**

### Rules

1. **JSON is the IPC contract.** C agents read JSON from stdin or a file path argument
   and write JSON to stdout. Python agents call C executables via subprocess and parse
   their stdout. No shared memory, no sockets, no temporary files for IPC.

2. **Every C agent validates its input.** Before processing, each agent checks that
   required JSON fields exist and have the expected types. On invalid input, the agent
   prints a JSON error object to stdout and exits with code 1.

3. **Every malloc has a matching free.** All dynamically allocated memory in C agents
   must be freed before exit. cJSON objects are freed with `cJSON_Delete()`. String
   buffers from `cJSON_Print()` are freed with `cJSON_free()` (or `free()`).

4. **No buffer overflows.** Use `fgets()` with explicit size limits, never `gets()`.
   Use `snprintf()` instead of `sprintf()`. String comparisons use `strncmp()` or
   `strcmp()` on validated, null-terminated strings only.

5. **Python agents are standalone functions.** Each Python agent exposes a single
   `run()` function callable from `main.py`. No shared global LLM client state.
   Each agent creates its own client using `llm_client.py` helpers and its own
   environment variable for the API key.

6. **Environment variables are never hardcoded.** API keys, model names, and base URLs
   are read from `.env` via `python-dotenv`. The `.env` file is never committed.
   `.env.example` documents all required variables.

7. **Python functions declare types.** All function signatures include type hints for
   parameters and return values. Dictionaries use descriptive keys documented in
   docstrings.

### How to verify

- Run C agents under Valgrind (Linux) or AddressSanitizer → zero leaks, zero errors.
- Feed malformed JSON to each C agent → must get a JSON error response, not a crash.
- Grep for `gets(`, `sprintf(` in C code → must find zero instances.
- Run `mypy` or manual review on Python agents → all functions have type hints.

---

## Project-Specific Context

### Architecture

```
CSV → data_agent (C) → analysis_agent (C) → classification_agent (C)
         ↓ JSON             ↓ JSON               ↓ JSON
    validated_data.json  analysis_results.json  fleet_status.json
                                                      ↓
    search_agent (Sonnet+RAG) → explanation_agent (Opus) → formatting_agent (GPT)
                                                                ↓
                                                         final_report.txt
                                                                ↓
                                                    coding_agent (GPT-4o-mini)
                                                                ↓
                                                     compliance_report.txt
```

### Language Responsibilities

| Language | Responsibility | Examples |
|----------|---------------|----------|
| C | Deterministic computation | KPIs, anomaly detection, classification |
| Python | LLM orchestration | RAG queries, prompt construction, API calls |
| JSON | Inter-agent communication | All agent inputs and outputs |
| Markdown | Documentation + RAG source | docs/, CLAUDE.md, skills/ |

### Domain Thresholds (Reference)

These are defined in `analysis_agent.c` and documented in `docs/thresholds.md`:

| Metric | Condition | Meaning |
|--------|-----------|---------|
| temperature | > 120 | Overheating |
| latency | > 800 | Excessive delay (ms) |
| error_rate | > 0.3 | Error rate beyond acceptable |
| battery_voltage | < 11.5 or > 13.0 | Voltage outside safe range |
| cpu_usage | > 90 | CPU saturation |
| memory_usage | > 85 | Memory pressure |
| gps_accuracy | > 5.0 | Poor localization |
| network_strength | ≤ -90 | Weak signal (dBm) |
| vibration | > 2.0 | Mechanical wear |
| wheel_speed_variance | > 15 | Traction issues |

### Classification Rules (Reference)

Defined in `classification_agent.c` and documented in `docs/classification_rules.md`:

| Status | Condition |
|--------|-----------|
| Critical | total_anomalies ≥ 20 **OR** metrics_affected ≥ 7 |
| Warning | total_anomalies ≥ 5 **OR** metrics_affected ≥ 3 |
| Healthy | Otherwise |

### File Naming Conventions

- C source: `snake_case.c` / `.h`
- Python modules: `snake_case.py`
- JSON outputs: `snake_case.json`
- Documentation: `snake_case.md`
- No spaces in file or directory names

### Commit Message Format

```
<session>: <component> — <what changed>

Session 3: agents_c — implement data_agent CSV parser
Session 5: agents_py — add search_agent RAG integration
```
