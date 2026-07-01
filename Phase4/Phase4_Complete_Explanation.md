# Phase 4 — Complete File-by-File Explanation
## Multi-Agent Telemetry with Guidelines, MCP, and RAG

> This document walks through every session and every file in `Phase4/`, explaining
> what each file does, why it exists, how it was built, and how it satisfies the
> project requirements. Nothing was changed; this is a read-only audit of the
> completed, verified implementation.

---

## Table of Contents

1. [Project Overview & Core Principle](#1-project-overview--core-principle)
2. [Session 1 — Scaffold](#2-session-1--scaffold)
3. [Session 2 — Guidelines (Three-Layer System)](#3-session-2--guidelines-three-layer-system)
4. [Session 3 — C Agents (Deterministic Core)](#4-session-3--c-agents-deterministic-core)
5. [Session 4 — RAG (ChromaDB + HuggingFace)](#5-session-4--rag-chromadb--huggingface)
6. [Session 5 — Python Agents (LLM Intelligence Layer)](#6-session-5--python-agents-llm-intelligence-layer)
7. [Session 6 — Orchestrator](#7-session-6--orchestrator)
8. [Session 7 — Lint & CI](#8-session-7--lint--ci)
9. [Session 8 — MCP & Docs](#9-session-8--mcp--docs)
10. [Verified Output Files](#10-verified-output-files)
11. [End-to-End Data Flow](#11-end-to-end-data-flow)

---

## 1. Project Overview & Core Principle

Phase 4 is the final and most sophisticated iteration of the Vehicle Robotics Operations pipeline. It solves a fundamental problem that existed in earlier phases: **LLMs were making decisions** (classifying vehicles, computing thresholds) instead of merely explaining them.

### The Golden Rule

```
Rules decide → LLM explains
```

- **C code** owns all data loading, KPI computation, anomaly detection, and health classification. It is deterministic, reproducible, and runs with zero API keys.
- **Python/LLM code** reads the JSON output of C agents and produces human-readable text. It never recomputes metrics, never re-evaluates thresholds, never changes a classification result.

### Technology Stack

| Layer | Technology | Purpose |
|-------|-----------|---------|
| Deterministic Core | C11 (gcc/clang) | Speed + correctness |
| LLM Intelligence | Python 3.12+ | Explanation + formatting |
| Inter-Agent Communication | JSON over stdin/stdout | Strict IPC contract |
| Embedding / Retrieval | ChromaDB + HuggingFace | RAG grounding |
| Guidelines | CLAUDE.md + skills + lint | Three-layer quality control |
| CI | GitHub Actions | Automated enforcement |
| IDE Integration | GitHub MCP (Cursor) | Developer tooling |

### Pipeline Flow (7 Agents)

```
sample_metrics.csv
       │
       ▼
[1] data_agent (C)           → outputs/validated_data.json
       │
       ▼
[2] analysis_agent (C)       → outputs/analysis_results.json
       │
       ▼
[3] classification_agent (C) → outputs/fleet_status.json
       │
       ▼
[4] search_agent (Sonnet+RAG)  ─┐
       │                         │ per vehicle
[5] explanation_agent (Opus)  ◄─┘
       │
       ▼
[6] formatting_agent (GPT-4o)  → outputs/final_report.md
       │
       ▼
[7] coding_agent (GPT-4o-mini) → outputs/compliance_report.txt
```

---

## 2. Session 1 — Scaffold

**Goal:** Create the entire Phase 4 directory tree, populate boilerplate files, and copy the source CSV data from Phase 3.

### `Phase4/data/sample_metrics.csv`

**What it is:** The input dataset for the entire pipeline — 25 rows of vehicle telemetry covering 3 vehicles (CAR_A, CAR_B, CAR_C) across 7 timestamps each.

**Why it matters:** Copied directly from Phase 3 so that the C agents can be verified for parity against the Python tools. The exact same numbers go in; the exact same KPIs and anomaly counts must come out.

**Structure:**

```
timestamp, vehicle_id, temperature, latency, error_rate, battery_voltage,
cpu_usage, memory_usage, gps_accuracy, network_strength, vibration, wheel_speed_variance
```

**Dataset design:** The three vehicles represent the three possible health outcomes:
- **CAR_A** — mostly normal readings, a couple of mild anomalies → **Healthy**
- **CAR_B** — one sharp temperature spike (128 °C), latency spike (840 ms), one 3-sigma network event → **Critical** (metrics_affected = 7)
- **CAR_C** — across-the-board extreme values, every metric well above thresholds → **Critical** (44 anomalies, 9 metrics affected)

### `Phase4/requirements.txt`

**What it is:** Python package manifest for the entire Phase 4 project.

**Contents explained:**

```
chromadb>=0.4.0            # Vector database for RAG storage
sentence-transformers>=2.2.0  # HuggingFace embedding model (~90MB first download)
openai>=1.0.0              # GPT-4o and GPT-4o-mini (Formatting + Coding agents)
anthropic>=0.25.0          # Claude Sonnet and Opus (Search + Explanation agents)
python-dotenv>=1.0.0       # Load .env API keys without hardcoding
pylint>=3.0.0              # Python linting (Layer 3)
pre-commit>=3.5.0          # Local git hook runner (Layer 3)
```

**Design decision:** All packages are pinned to minimum-version (`>=`) rather than exact-version (`==`) to stay compatible with future security patches while avoiding breaking changes.

### `Phase4/.gitignore`

**What it is:** Tells Git which files must never be committed.

**Key exclusions:**

| Pattern | Reason |
|---------|--------|
| `.env` | Contains API keys — must never be pushed to any repository |
| `vector_store/` | ChromaDB index is rebuilt from `docs/` on demand; no need to commit binary blobs |
| `__pycache__/`, `*.pyc` | Python bytecode — machine-generated, environment-specific |
| `*.exe`, `*.o`, `*.out` | C build artifacts — rebuilt from source with `build.bat`/`make` |
| `outputs/*.json`, `outputs/*.txt`, `outputs/*.md` | Pipeline outputs — regenerated on every run |

### `Phase4/.env.example`

**What it is:** A template showing every environment variable the project needs, with empty values. Developers copy this to `.env` and fill in their keys.

**Variables:**

```bash
ANTHROPIC_API_KEY=   # Search Agent (Sonnet) + Explanation Agent (Opus)
OPENAI_API_KEY=      # Formatting Agent (GPT-4o) + Coding Agent (GPT-4o-mini)
OPENROUTER_API_KEY=  # Optional alternative unified endpoint
GITHUB_TOKEN=        # GitHub MCP for Cursor (CI status, issues, PRs)
```

**Design rule (from CLAUDE.md Guideline 2):** API keys are **never** hardcoded anywhere in source files. All agents read from environment variables via `os.getenv()` after `load_dotenv()`.

---

## 3. Session 2 — Guidelines (Three-Layer System)

**Goal:** Implement the complete three-layer coding guidelines infrastructure before writing any agent code. This ensures every future line of code is written against known rules.

### Layer 1: `Phase4/CLAUDE.md` — The Law Book

**What it is:** The master rule document that governs all agents, both human and AI. It is the single source of truth for project architecture, language responsibilities, and coding rules.

**Structure:**

#### Coding Guideline 1 — Separation of Deterministic Logic and LLM Reasoning

This is the most important rule in the entire project:

> All data processing, metric computation, anomaly detection, and health classification must be implemented in deterministic, reproducible code. LLMs are never used to make decisions — they only explain decisions already made by rule-based logic.

The five specific rules under this guideline are:

1. **C agents own all decisions** — `data_agent.c`, `analysis_agent.c`, `classification_agent.c` are the only code that computes anything.
2. **Python agents own all explanations** — they consume JSON and produce text only.
3. **No LLM in the critical path** — the C pipeline must produce valid output even if all API keys are missing.
4. **Thresholds live in C only** — temperature > 120, latency > 800, etc., are evaluated exclusively in `analysis_agent.c`.
5. **Classification rules live in C only** — the Healthy/Warning/Critical boundaries are evaluated exclusively in `classification_agent.c`.

#### Coding Guideline 2 — Strict Interface Contracts and Memory Safety

Seven rules covering IPC and C memory discipline:

1. JSON is the IPC contract (stdin/stdout, no shared memory).
2. Every C agent validates its input before processing.
3. Every `malloc` has a matching `free`; every `cJSON_Parse` result is freed with `cJSON_Delete`.
4. No buffer overflows — use `fgets()`, `snprintf()`, never `gets()`, `sprintf()`.
5. Python agents are standalone functions with no global LLM client state.
6. Environment variables are never hardcoded.
7. Python functions declare type hints.

#### Project-Specific Context

The final section of CLAUDE.md provides an ASCII architecture diagram, a language responsibility table, the domain threshold reference table, the classification rule reference table, file naming conventions, and the commit message format.

**Why it comes before any code:** CLAUDE.md is the rulebook that the Coding Agent (GPT-4o-mini) reads to validate the final report. It's also the first thing any new contributor or AI agent must read. Writing it first ensures that all subsequent code is tested against known rules.

---

### Layer 2: `Phase4/skills/SKILL.md` — Self-Check Playbooks

**What it is:** A set of task-specific checklists (called "playbooks") that a developer or AI agent runs through before completing a task. While CLAUDE.md states the laws, SKILL.md provides the step-by-step procedure for following them.

**Five playbooks:**

#### Playbook A — Writing C Agents
Triggered whenever modifying anything in `agents_c/`. Checks include:
- No LLM calls anywhere in the C code
- JSON input validation is present
- JSON output matches the documented schema
- Every `malloc` has a matching `free`
- No unsafe functions (`gets`, `sprintf`)
- All 10 domain thresholds match CLAUDE.md exactly
- Classification rules match CLAUDE.md exactly
- Builds clean under `-Wall -Wextra`
- Post-build parity check against Phase 3 Python output

#### Playbook B — Writing Python Agents
Triggered whenever modifying anything in `agents_py/`. Checks include:
- No anomaly re-computation in Python
- Single standalone `run()` function per agent
- Correct model assignment (Sonnet for search, Opus for explanation, GPT-4o for formatting, GPT-4o-mini for coding)
- Type hints on all function signatures
- Docstrings with Args and Returns
- Graceful degradation when API key is missing

#### Playbook C — Running the Full Pipeline
Triggered when running `python main.py`. Verifies prerequisites (C agents built, RAG index exists, `.env` configured) and execution order (7 steps in sequence).

#### Playbook D — Modifying Documentation or RAG Sources
Triggered when changing any file in `docs/`. Ensures threshold and rule consistency between documentation and C code, and requires a RAG index rebuild after any doc change.

#### Playbook E — Lint and CI Changes
Triggered when modifying `.clang-format`, `.pylintrc`, `.pre-commit-config.yaml`, or `lint.yml`. Ensures no false positives are introduced and that CI mirrors local checks exactly.

---

## 4. Session 3 — C Agents (Deterministic Core)

**Goal:** Port Phase 3's Python telemetry processing tools exactly into three C agents communicating via JSON IPC. No LLM, no Python, no external libraries except the vendored cJSON.

### `Phase4/agents_c/json_utils.h` — Vendored cJSON Header

**What it is:** A self-contained C header declaring the cJSON library — a lightweight, public-domain JSON parser/generator. It is "vendored" (included directly in the repo) so that the build has no external dependencies.

**Why cJSON instead of hand-written JSON:** The anomaly output is a nested JSON object (vehicle → metric → array of values). Correctly escaping and formatting nested arrays by hand in C is extremely error-prone. cJSON handles all of that safely.

**Key declarations:**

```c
typedef struct cJSON {
    struct cJSON *next, *prev, *child;
    int type;           // cJSON_String, cJSON_Number, cJSON_Array, etc.
    char *valuestring;  // value if type == cJSON_String
    double valuedouble; // value if type == cJSON_Number
    char *string;       // key name if this is a child of an object
} cJSON;
```

**Memory management pattern:**
- `cJSON_Parse(str)` → returns heap-allocated tree; must be freed with `cJSON_Delete()`
- `cJSON_Print(item)` → returns heap-allocated string; must be freed with `cJSON_free()`

**Iteration macro:**
```c
#define cJSON_ArrayForEach(element, array) \
    for (element = (array != NULL) ? (array)->child : NULL; \
         element != NULL; element = element->next)
```
This macro is used throughout all three C agents to iterate over both JSON arrays and JSON objects (cJSON represents both as a linked list of children).

### `Phase4/agents_c/json_utils.c` — Vendored cJSON Implementation

**What it is:** The full implementation of the cJSON library (~600 lines of C). It implements:
- The recursive descent JSON parser (`cJSON_Parse`)
- The pretty-printer (`cJSON_Print`) and compact printer (`cJSON_PrintUnformatted`)
- All creation functions (`cJSON_CreateNumber`, `cJSON_CreateString`, etc.)
- All query/mutation functions (`cJSON_GetObjectItem`, `cJSON_AddItemToArray`, etc.)
- All type-check functions (`cJSON_IsNumber`, `cJSON_IsString`, etc.)

**Build rule:** Every C agent is compiled by linking against `json_utils.c`:
```
gcc -Wall -Wextra -std=c11 -O2 -o data_agent data_agent.c json_utils.c -lm
```

---

### `Phase4/agents_c/data_agent.c` — Data Loading Agent

**Ported from:** `Phase 3/tools/load_data.py`

**Role:** First agent in the pipeline. Reads the raw CSV file, validates every row, and outputs structured JSON grouping rows by vehicle ID.

**Input:** CSV file path as a command-line argument.

**Output (JSON to stdout):**
```json
{
  "status": "success",
  "vehicles": ["CAR_A", "CAR_B", "CAR_C"],
  "columns": ["timestamp", "vehicle_id", "temperature", ...],
  "data": {
    "CAR_A": [
      {"timestamp": "2024-01-01 00:00:00", "vehicle_id": "CAR_A", "temperature": 74.0, ...}
    ],
    ...
  },
  "total_rows": 21,
  "validation": {"valid_rows": 21, "skipped_rows": 0}
}
```

**Key implementation details:**

| Function | Purpose |
|---------|---------|
| `trim(char *str)` | Removes leading/trailing whitespace and `\r\n` in-place. Critical for Windows CRLF CSV files. |
| `split_csv_line(...)` | Splits a CSV line into fields, respecting the `MAX_FIELD_LEN=256` bound per field. |
| `is_number(const char *str)` | Validates numeric strings before calling `atof()`, preventing undefined behavior. |
| `error_exit(const char *msg)` | Prints a `{"status":"error","message":"..."}` JSON to stdout and exits with code 1. |

**Memory safety:**
- All arrays are stack-allocated with fixed bounds (`MAX_COLUMNS=64`, `MAX_FIELD_LEN=256`).
- `cJSON_Delete(root)` is called before return to free the entire output tree.
- `cJSON_free(output)` is called immediately after `printf` to free the serialized string.

**Error handling:**
- Missing `vehicle_id` column → JSON error + exit 1
- Row with wrong field count → skipped (counted in `skipped_rows`)
- Empty vehicle_id → skipped

---

### `Phase4/agents_c/analysis_agent.c` — Metrics Analysis Agent

**Ported from:** `Phase 3/tools/analyze_metrics.py`

**Role:** Second agent. Reads the validated data JSON from Agent 1, and for each vehicle and each of 10 numeric metrics: computes KPIs (mean, min, max, std) and detects anomalies using both the 3-sigma rule and domain-specific thresholds.

**Input:** `validated_data.json` (from data_agent), via file path argument or stdin.

**Output (JSON to stdout):**
```json
{
  "status": "success",
  "results": {
    "CAR_A": {
      "kpis": {
        "temperature": {"mean": 77.0, "min": 74.0, "max": 80.0, "std": 2.16},
        ...
      },
      "anomalies": {
        "temperature": [],
        "latency": [820.0, 910.0],
        ...
      }
    },
    ...
  }
}
```

**The 10 numeric metrics (order matters for iteration):**
```c
static const char *NUMERIC_METRICS[] = {
    "temperature", "latency", "error_rate", "battery_voltage",
    "cpu_usage", "memory_usage", "gps_accuracy", "network_strength",
    "vibration", "wheel_speed_variance", NULL
};
```

**Statistical functions:**
- `compute_mean(values, count)` — sum / count
- `compute_min(values, count)` — linear scan
- `compute_max(values, count)` — linear scan
- `compute_std(values, count)` — sample standard deviation (divides by `count - 1`, matching pandas `ddof=1`)

**Domain threshold checker (`check_domain_threshold`):**

This is the most critical function in the entire project. These exact 10 comparisons are the business rules for the fleet:

```c
if (strcmp(metric, "temperature") == 0)       return value > 120.0;
if (strcmp(metric, "latency") == 0)           return value > 800.0;
if (strcmp(metric, "error_rate") == 0)        return value > 0.3;
if (strcmp(metric, "battery_voltage") == 0)   return (value < 11.5) || (value > 13.0);
if (strcmp(metric, "cpu_usage") == 0)         return value > 90.0;
if (strcmp(metric, "memory_usage") == 0)      return value > 85.0;
if (strcmp(metric, "gps_accuracy") == 0)      return value > 5.0;
if (strcmp(metric, "network_strength") == 0)  return value <= -90.0;
if (strcmp(metric, "vibration") == 0)         return value > 2.0;
if (strcmp(metric, "wheel_speed_variance") == 0) return value > 15.0;
```

**Anomaly combination logic:** For every data point, both the 3-sigma check and the domain threshold check are performed. A value is added to the anomaly list if either check is true — this is a union (`is_statistical || is_domain`). The `value_in_array` function with `fabs(a - b) < 1e-10` epsilon comparison prevents the same value appearing twice.

**Memory safety:**
- Input is read into a 1 MB stack-allocated buffer via `read_input()`.
- Input JSON tree is freed with `cJSON_Delete(input)` after processing.
- Output JSON tree is freed with `cJSON_Delete(root)` after printing.

---

### `Phase4/agents_c/classification_agent.c` — Vehicle Health Classification Agent

**Ported from:** `Phase 3/tools/classify_vehicle.py`

**Role:** Third and final C agent. Reads the analysis results JSON and applies the fleet classification rules to assign each vehicle a health status.

**Input:** `analysis_results.json` (from analysis_agent), via file path argument or stdin.

**Output (JSON to stdout):**
```json
{
  "status": "success",
  "fleet_status": {
    "CAR_A": {"status": "Healthy", "total_anomalies": 2, "metrics_affected": 1},
    "CAR_B": {"status": "Critical", "total_anomalies": 8, "metrics_affected": 7},
    "CAR_C": {"status": "Critical", "total_anomalies": 44, "metrics_affected": 9}
  }
}
```

**Classification algorithm:**
```c
// Count total anomalies and metrics affected
cJSON_ArrayForEach(metric_anomalies, anomalies) {
    int count = cJSON_GetArraySize(metric_anomalies);
    total_anomalies += count;
    if (count > 0) metrics_affected++;
}

// Apply rules in priority order
if (total_anomalies >= 20 || metrics_affected >= 7) health_status = "Critical";
else if (total_anomalies >= 5  || metrics_affected >= 3) health_status = "Warning";
else health_status = "Healthy";
```

**Why the priority order matters:** The rules are evaluated top-down. Critical is checked first, so a vehicle with 25 anomalies across 8 metrics is always Critical — it will never be downgraded to Warning.

**Verification of the actual output:**
- CAR_A: 2 anomalies, 1 metric affected → **Healthy** ✓ (< 5 anomalies and < 3 metrics)
- CAR_B: 8 anomalies, 7 metrics affected → **Critical** ✓ (metrics_affected ≥ 7 triggers it)
- CAR_C: 44 anomalies, 9 metrics affected → **Critical** ✓ (both conditions met)

---

### `Phase4/Makefile` — Unix/macOS/Linux Build

**What it is:** A GNU Make build script with three targets.

**Targets:**
- `make` or `make all` — compiles all three C agents
- `make clean` — deletes compiled binaries and `.o` files
- `make test` — builds then runs the C-only pipeline, writing all three JSON outputs

**Compile flags:** `-Wall -Wextra -std=c11 -O2` — full warnings, C11 standard, optimization level 2. `-lm` links the math library needed for `sqrt()` in `analysis_agent.c`.

**Dependency tracking:** Each agent target lists its `.c` and `.h` dependencies, so `make` only recompiles what changed.

### `Phase4/build.bat` — Windows Build

**What it is:** A Windows batch script providing identical functionality to the Makefile.

**Three modes:**
- `build.bat` — build all agents (produces `*.exe` files)
- `build.bat clean` — delete `*.exe` and `*.o` files
- `build.bat test` — build then run all three C agents on sample data

**Windows-specific notes:**
- Uses `%CC%` and `%CFLAGS%` variables for the same gcc flags as the Makefile.
- Outputs go to `agents_c\data_agent.exe`, etc.
- `IF ERRORLEVEL 1` checks after each compilation bail out early on failure.
- Requires MinGW (which provides `gcc.exe` on Windows) — documented in the README.

---

## 5. Session 4 — RAG (ChromaDB + HuggingFace)

**Goal:** Build the Retrieval-Augmented Generation subsystem that grounds LLM responses in project documentation. This prevents the Explanation Agent from hallucinating thresholds or rules.

### `Phase4/docs/thresholds.md` — Domain Thresholds Reference

**What it is:** A human-readable markdown document that describes every domain threshold used by `analysis_agent.c`, plus the statistical anomaly detection method.

**Why it exists:** Two purposes:
1. It is the primary RAG source document — when the Search Agent queries for "temperature threshold," this document is what ChromaDB retrieves.
2. It documents the thresholds for human contributors so they can verify `analysis_agent.c` implements them correctly.

**Content covers all 10 metrics:**
- Each metric has: condition, meaning, normal range
- The statistical (3-sigma) method is documented separately
- The anomaly combination (union) logic is documented

**Sync requirement (Playbook D):** If `thresholds.md` is modified, the values in `analysis_agent.c` and `CLAUDE.md` must also be updated, and `rag_setup.py` must be re-run to rebuild the index.

---

### `Phase4/docs/classification_rules.md` — Health Classification Rules Reference

**What it is:** A markdown document defining the Healthy/Warning/Critical classification rules, with examples and edge-case explanations.

**Content:**

- **Critical:** `total_anomalies ≥ 20 OR metrics_affected ≥ 7` — vehicle must be taken out of service
- **Warning:** `total_anomalies ≥ 5 OR metrics_affected ≥ 3` — monitor closely, schedule maintenance
- **Healthy:** Otherwise — continue routine monitoring

**Key clarifications in the document:**
- Priority order (Critical checked first, then Warning)
- Anomaly counting details (individual data points, not unique types)
- Both statistical and domain anomalies count, after deduplication

---

### `Phase4/docs/architecture.md` — Pipeline Architecture Reference

**What it is:** A comprehensive markdown overview of the entire Phase 4 system — the design principle, pipeline ASCII diagram, agent details table, RAG subsystem description, IPC contract, and guidelines layer summary.

**Why it is in `docs/` (not just README):** It is indexed by RAG so the Explanation Agent can retrieve it when generating insights that reference the system architecture. It is also the most complete reference for understanding how all the pieces fit together.

---

### `Phase4/docs/.gitkeep`

**What it is:** An empty-ish placeholder file (with a comment) that forces Git to track the `docs/` directory even before any markdown files are present. Git does not track empty directories.

---

### `Phase4/rag/rag_setup.py` — RAG Index Builder

**What it is:** A standalone Python script that walks the `docs/` directory, chunks all markdown files, embeds the chunks using HuggingFace, and persists the index to `vector_store/` via ChromaDB.

**Run once before the pipeline (or after doc changes):**
```bash
python rag/rag_setup.py
```

**Key functions:**

#### `chunk_text(text, chunk_size=500, overlap=50)`
Splits a document into overlapping chunks using whitespace-based token counting:
- Splits text into words (approximate tokens)
- Creates chunks of ~500 words, advancing by `500 - 50 = 450` words each time
- The 50-word overlap ensures that sentences at chunk boundaries are not split across two disconnected chunks

#### `load_documents(docs_dir)`
Walks `docs/` recursively for `*.md` files, reads each, chunks it, and returns a flat list of dicts:
```python
{"text": chunk_str, "source": "thresholds.md", "chunk_index": 0}
```
Files starting with `.` (like `.gitkeep`) are skipped.

#### `build_index(docs_dir, persist_dir)`
The main function:
1. Loads documents
2. Creates a `SentenceTransformerEmbeddingFunction` using `all-MiniLM-L6-v2` (~90MB, downloaded on first run)
3. Opens a `chromadb.PersistentClient` writing to `vector_store/`
4. Deletes the existing `phase4_docs` collection if it exists (idempotent)
5. Creates a fresh collection and adds all chunks with their metadata
6. Prints a summary of what was indexed

**Why idempotent rebuild instead of incremental update:** Simpler and safer. Re-running `rag_setup.py` always produces a consistent, known-good index regardless of what may have been in the old index.

**First-run note:** The sentence-transformers model download is ~90MB. After the first run it is cached locally and subsequent runs are fast.

---

### `Phase4/rag/rag_query.py` — RAG Query Interface

**What it is:** A module that provides two query functions used by the Search Agent at runtime.

#### `query_rag(question, top_k=3, persist_dir="vector_store")`
Simple query returning just text chunks:
- Checks if `vector_store/` exists; warns and returns `[]` if not
- Checks if the `phase4_docs` collection exists
- Runs `collection.query(query_texts=[question], n_results=top_k)`
- Returns a list of text strings

#### `query_rag_with_metadata(question, top_k=3, persist_dir="vector_store")`
Richer query returning chunks with source provenance:
- Returns a list of dicts: `{"text": ..., "source": "thresholds.md", "distance": 0.32}`
- The `distance` is the cosine distance from ChromaDB — lower = more relevant
- Used by `search_agent.py` because the source filename is included in the context shown to the Explanation Agent

**Graceful degradation:** Both functions:
- Catch `ImportError` (chromadb not installed) and return `[]`
- Check directory and collection existence, returning `[]` with a warning instead of crashing

**Standalone test mode:**
```bash
python rag/rag_query.py temperature threshold
```
Prints the top-3 retrieved chunks with source and distance information.

---

### `Phase4/rag/__init__.py`

**What it is:** Empty Python package marker. Makes `rag/` a proper Python package so that `from rag.rag_query import query_rag_with_metadata` works from `main.py`.

---

## 6. Session 5 — Python Agents (LLM Intelligence Layer)

**Goal:** Implement four Python LLM agents, each using a specific model, each a standalone function, each reading only from C agent JSON outputs and producing text. Also implement the shared `llm_client.py` with full mock mode for offline development.

### `Phase4/agents_py/__init__.py`

**What it is:** Python package marker. Allows `from agents_py.search_agent import retrieve_context_for_vehicle` to work from `main.py`.

---

### `Phase4/agents_py/llm_client.py` — Universal LLM Interface

**What it is:** The shared utility module that all four Python agents use to make API calls. Its two primary features are unified interface and mock mode.

**The `call_llm(prompt, provider, model, system_prompt=None)` function:**

```python
def call_llm(prompt: str, provider: str, model: str, system_prompt: Optional[str] = None) -> str:
    mock_mode = not os.getenv("ANTHROPIC_API_KEY") and not os.getenv("OPENAI_API_KEY")
    if mock_mode:
        return _mock_llm(prompt, provider, model)
    if provider.lower() == "anthropic":
        return _call_anthropic(prompt, model, system_prompt)
    elif provider.lower() == "openai":
        return _call_openai(prompt, model, system_prompt)
```

**Design decision — mock mode evaluated at call time, not import time:** The `load_dotenv()` in `main.py` runs before any agents are called. If mock mode were evaluated at import time, it would not see the `.env` variables. Checking `os.getenv()` inside `call_llm()` means the mock check happens after dotenv has populated the environment.

**`_call_anthropic(prompt, model, system_prompt)`:** Uses the `anthropic` package, creates a new `Anthropic()` client per call (no shared global state — satisfies CLAUDE.md Guideline 2, Rule 5), sends a single-turn message, returns `response.content[0].text`.

**`_call_openai(prompt, model, system_prompt)`:** Uses the `openai` package, creates a new `OpenAI()` client per call, sends a `chat.completions.create()` request, returns `response.choices[0].message.content`.

**`_mock_llm(prompt, provider, model)`:** Returns realistic, hardcoded responses keyed on keywords in the prompt:
- Prompts containing "search queries" → returns search query strings for the Search Agent
- Prompts containing "write a concise technical insight" → returns structured insight text for CAR_A, CAR_B, or CAR_C
- Prompts containing "format the final fleet" → returns a mock markdown report for the Formatting Agent
- Prompts containing "validate" or "compliance" → returns a COMPLIANT verdict for the Coding Agent

This mock mode allows the entire 7-agent pipeline to run completely offline without any API keys, producing realistic-looking (if deterministic) outputs.

---

### `Phase4/agents_py/search_agent.py` — RAG Search Agent

**Model:** Claude Sonnet 4.6 (`claude-sonnet-4-6`)
**Provider:** Anthropic

**Role:** The bridge between the deterministic C layer and the LLM explanation layer. For each vehicle with anomalies, it:
1. Asks Sonnet to generate 2 natural language search queries based on the vehicle's anomalous metrics
2. Executes those queries against the ChromaDB RAG index
3. Returns the retrieved documentation chunks to be passed to the Explanation Agent

**The `retrieve_context_for_vehicle(vehicle_id, anomalies)` function:**

```python
prompt = f"""You are a search assistant for vehicle telemetry.
The vehicle {vehicle_id} has anomalies in the following metrics: {metrics_list}

Extract exactly 2 short search queries to find the relevant thresholds and rules 
for these metrics in our documentation.
Format your response as a comma-separated list of queries, nothing else.
"""
```

**Why use an LLM to generate search queries instead of querying directly?** The metric names from the C agent (e.g., "wheel_speed_variance") may not match the phrasing in the documentation ("wheel speed variance" or "traction issues"). Sonnet can translate between the technical field names and the natural language phrases used in the markdown docs.

**Fallback:** If Sonnet fails to return a comma-separated list, the agent falls back to `"{metric} threshold"` for each anomalous metric name.

**Deduplication:** A `seen_texts` set ensures that the same chunk is never returned twice, even if multiple queries return overlapping results.

---

### `Phase4/agents_py/explanation_agent.py` — Telemetry Explanation Agent

**Model:** Claude Opus 4.8 (`claude-opus-4-8`)
**Provider:** Anthropic

**Role:** The most sophisticated LLM agent. It receives the full context for one vehicle (status, KPIs, anomaly list, RAG context) and generates a structured technical insight explaining what is happening and why.

**The `generate_vehicle_insight(vehicle_id, status_info, kpis, anomalies, rag_context)` function:**

The prompt is carefully constructed with an explicit constraint embedded in the text:

```
IMPORTANT CONSTRAINT (CLAUDE.md): You must NOT classify the vehicle or alter the metrics.
Your ONLY job is to EXPLAIN the deterministic results provided below, grounding your
explanation in the provided documentation context.
```

**Output format enforced by the prompt:**
```
1) What is happening (1-3 sentences)
2) Likely causes (bullet list, max 3)
3) Recommended next actions (bullet list, max 3)
```

**Why Opus and not Sonnet?** Opus is Anthropic's most capable model. The explanation task requires nuanced understanding of automotive/robotics systems — connecting a `gps_accuracy > 5.0` anomaly to likely root causes (e.g., signal obstruction, antenna failure) requires deep technical reasoning. Sonnet handles the simpler search/query task; Opus handles the complex reasoning task.

**Temperature set to 0.2** (in `llm_client.py`): Keeps responses factual and consistent across runs, reducing hallucination risk.

---

### `Phase4/agents_py/formatting_agent.py` — Report Formatting Agent

**Model:** GPT-4o (`gpt-4o`)
**Provider:** OpenAI

**Role:** Takes all the pipeline data (fleet status from C, analysis results from C, insights from Opus) and compiles them into a final, professional markdown report.

**The `format_final_report(fleet_status, analysis_results, insights)` function:**

The prompt gives GPT-4o a compact JSON summary:
```python
summary_data = [
    {"vehicle": "CAR_A", "status": "Healthy", "anomalies": 2, "insight": "..."},
    ...
]
```

**The key formatting instructions:**
1. Start with `# Phase 4 Fleet Telemetry Report` header
2. Write a 1-paragraph executive summary
3. Create a `## Vehicle ID (Status)` subsection for each vehicle
4. Include total anomalies
5. Include the **exact** insight text — `Do NOT alter the insight text`

**Why GPT for formatting instead of Anthropic?** OpenAI's GPT-4o excels at structured document generation and formatting. Using it for formatting while using Anthropic for reasoning also demonstrates multi-provider architecture.

---

### `Phase4/agents_py/coding_agent.py` — Guidelines Validation Agent

**Model:** GPT-4o-mini (`gpt-4o-mini`)
**Provider:** OpenAI

**Role:** The final quality gate. It reads `CLAUDE.md` and the final report, then checks whether the LLM-generated report violates the guidelines.

**The `validate_report(report_text)` function:**

It dynamically reads `CLAUDE.md` at runtime:
```python
claude_md_path = os.path.join(project_root, "CLAUDE.md")
with open(claude_md_path, "r", encoding="utf-8") as f:
    guidelines = f.read()
guidelines_excerpt = guidelines[:2000]  # First 2000 chars to save tokens
```

**Validation checks in the prompt:**
1. "Rules decide → LLM explains": Does the report present health status as a hard fact rather than a guess?
2. Did the LLM invent sensors or data not present in standard telemetry?

**Output format:** Must start with `"COMPLIANT:"` or `"NON-COMPLIANT:"` followed by 1-2 sentences of justification.

**Why GPT-4o-mini?** The compliance check is a relatively simple reading comprehension task — does the text follow the rules? GPT-4o-mini is faster and cheaper, appropriate for this validation step.

---

## 7. Session 6 — Orchestrator

**Goal:** Build `main.py` to chain all 7 agents in sequence, managing data flow between C executables and Python functions.

### `Phase4/main.py` — Pipeline Orchestrator

**What it is:** The single entry point for the entire Phase 4 pipeline. Running `python main.py` executes all 7 agents in order.

**The `run_c_agent(executable_path, input_arg=None, input_data=None)` helper:**

This function abstracts the subprocess mechanics:
- Detects Windows (appends `.exe` if needed)
- Constructs the command: `[executable]` optionally followed by `[input_arg]`
- Runs the subprocess, capturing stdout and stderr
- Parses the stdout as JSON with `json.loads()`
- On any failure (file not found, non-zero exit, JSON parse error) → prints a clear error and exits

**IPC pattern used by each C agent:**
- `data_agent`: file path as argument → `run_c_agent(exe, input_arg=csv_path)`
- `analysis_agent`: validated data via stdin → `run_c_agent(exe, input_data=json.dumps(validated_data))`
- `classification_agent`: analysis results via stdin → `run_c_agent(exe, input_data=json.dumps(analysis_results))`

**The `main()` function — 7 steps:**

```
[1/7] Data Agent (C)          → validated_data.json
[2/7] Analysis Agent (C)      → analysis_results.json
[3/7] Classification Agent (C) → fleet_status.json
[4/7] Search Agent (Sonnet+RAG) — per vehicle
[5/7] Explanation Agent (Opus)  — per vehicle
[6/7] Formatting Agent (GPT-4o) → final_report.md
[7/7] Coding Agent (GPT-4o-mini) → compliance_report.txt
```

**Steps 4 and 5 are combined in a single loop** over all vehicles:
```python
for vid in vehicles:
    rag_context = retrieve_context_for_vehicle(vid, anomalies)
    insight = generate_vehicle_insight(vid, v_status, kpis, anomalies, rag_context)
    all_insights[vid] = insight
```

This design means the Search Agent and Explanation Agent are called once per vehicle rather than once for the whole fleet — keeping each call's context focused.

**`load_dotenv()`** is called at the top of `main()`, before any agents run, so that API keys are available before the mock-mode check in `llm_client.py`.

**Output files written:** All 5 outputs are explicitly written by `main.py` using `json.dump()` and `open().write()`. The C agents write to stdout; `main.py` captures stdout (via `run_c_agent`) and writes the files. The Python agents return strings; `main.py` writes those strings to files.

---

## 8. Session 7 — Lint & CI

**Goal:** Implement Layer 3 of the guidelines system — automated tooling that enforces CLAUDE.md rules without requiring manual checklist review.

### `Phase4/.clang-format` — C Style Enforcement

**What it is:** Configuration for `clang-format`, the industry-standard C/C++ formatter.

**Settings:**

| Setting | Value | Reason |
|---------|-------|--------|
| `BasedOnStyle` | Google | Well-known, consistent style |
| `Standard` | c11 | Matches compile target |
| `ColumnLimit` | 80 | Traditional terminal width |
| `IndentWidth` | 4 | Better readability for nested code |
| `UseTab` | Never | Consistent across all editors |
| `BreakBeforeBraces` | Attach | K&R style (`if (x) {`) |
| `PointerAlignment` | Right | `char *str` not `char* str` |
| `SortIncludes` | true | Deterministic include order |
| `AlignTrailingComments` | true | Aligns `/* comments */` vertically |

**How it is enforced:** `clang-format --style=file` reads this file automatically. The pre-commit hook runs `clang-format -i` on all `.c` and `.h` files. The CI job diffs the formatted output against the actual file and fails if they differ.

---

### `Phase4/.pylintrc` — Python Style Enforcement

**What it is:** Configuration for `pylint`, the standard Python static analysis tool.

**Key sections:**

**`[MESSAGES CONTROL]` — disabled warnings:**
- `missing-module-docstring` — not required for small modules
- `import-error` — disabled because `anthropic` and `openai` are optional (mock mode)
- `import-outside-toplevel` — disabled because `llm_client.py` imports these conditionally
- `broad-except` — allowed for graceful degradation patterns

**`[FORMAT]`:** `max-line-length=100` (slightly relaxed from PEP8's 79 for modern screens)

**`[BASIC]`:** `good-names` adds project-specific short names: `vid` (vehicle ID), `ef` (embedding function), `db` (database), etc.

**`[DESIGN]`:** `max-args=8` (relaxed from default 5 to accommodate the multi-parameter agent functions like `generate_vehicle_insight`)

---

### `Phase4/.pre-commit-config.yaml` — Local Git Hook Runner

**What it is:** Configuration for `pre-commit`, a tool that runs checks automatically on `git commit` before the commit is allowed.

**Three hook repositories:**

```yaml
# 1. General file hygiene
- repo: https://github.com/pre-commit/pre-commit-hooks
  hooks:
    - trailing-whitespace   # No spaces at end of lines
    - end-of-file-fixer     # Files end with a single newline
    - check-yaml            # YAML syntax validation
    - check-json            # JSON syntax validation
    - check-added-large-files  # Blocks accidental binary blobs

# 2. Python linting
- repo: https://github.com/pylint-dev/pylint
  hooks:
    - pylint --rcfile=Phase4/.pylintrc
    - files: ^Phase4/.*\.py$   # Only Phase4 Python files

# 3. C formatting
- repo: https://github.com/pre-commit/mirrors-clang-format
  hooks:
    - clang-format --style=file
    - files: ^Phase4/agents_c/.*\.(c|h)$  # Only Phase4 C files
```

**How to install:**
```bash
pip install pre-commit
pre-commit install
```

After installation, every `git commit` automatically runs all three hooks and blocks the commit if any fail.

---

### `Phase4/.github/workflows/lint.yml` — CI Lint Workflow

**What it is:** A GitHub Actions workflow that runs the same checks as the pre-commit hooks, but in the cloud on every push or pull request touching `Phase4/**`.

**Three parallel jobs:**

#### Job 1: `clang-format` (C Style)
```yaml
runs-on: ubuntu-latest
steps:
  - Install clang-format 17
  - Find all *.c and *.h files in agents_c/
  - For each file: diff clang-format output vs actual file
  - If different: error annotation pointing to the file
```

#### Job 2: `pylint` (Python Style)
```yaml
runs-on: ubuntu-latest
steps:
  - Set up Python 3.12
  - pip install pylint>=3.0.0
  - Run pylint with .pylintrc on all 7 Python source files
```

#### Job 3: `whitespace` (Trailing Whitespace)
```yaml
runs-on: ubuntu-latest
steps:
  - grep -rn " $" across all .c, .h, .py, .md, .yaml, .yml files
  - Fail if any trailing whitespace is found
```

**Key design principle — CI mirrors local:** The pre-commit hooks and the CI workflow run identical checks. There is no rule that runs in CI but not locally, and vice versa. This means if `pre-commit run --all-files` passes locally, the CI will pass.

---

## 9. Session 8 — MCP & Docs

**Goal:** Add the GitHub MCP configuration for Cursor IDE integration, write the Phase 4 README, and update the root README with the Phase 4 section.

### `Phase4/mcp-config.json` — GitHub MCP for Cursor

**What it is:** A Model Context Protocol (MCP) server configuration file that tells Cursor IDE how to connect to the GitHub MCP server.

**Content:**
```json
{
  "mcpServers": {
    "github": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-github"],
      "env": {
        "GITHUB_PERSONAL_ACCESS_TOKEN": "${GITHUB_TOKEN}"
      },
      "description": "GitHub MCP server for Cursor. ..."
    }
  }
}
```

**Intended uses (from the description field):**
1. **Read CI status** — Query `lint.yml` workflow runs to see if the latest push passed
2. **Create issues for Critical vehicles** — When the pipeline finds Critical vehicles, create GitHub issues for the fleet operations team
3. **Open PRs for report artifacts** — Submit the generated `final_report.md` as a PR artifact

**Critical distinction:** MCP is **Cursor-side configuration only**. It is never imported or called from `main.py`. It is a developer tool, not a runtime dependency. The pipeline runs identically whether or not Cursor MCP is configured.

**Token handling:** `"${GITHUB_TOKEN}"` is a template variable that Cursor resolves from the local environment. The actual token comes from `.env` (gitignored), never from this config file.

---

### `Phase4/README.md` — Phase 4 Module README

**What it is:** The primary documentation for the Phase 4 module, oriented toward developers who want to understand and run the pipeline.

**Sections:**
1. **Architecture overview** — Two-paragraph summary of the "Rules decide → LLM explains" principle
2. **Deterministic Core (C11)** — Description of all three C agents
3. **Intelligence Layer (Python + RAG)** — Description of all four Python agents
4. **Getting Started** — Prerequisites (Python 3.12+, GCC/Clang, Make)
5. **Installation & Setup** — Three steps: `pip install`, RAG setup, C build
6. **Running the Pipeline** — `python main.py` with a note about Mock Mode
7. **Output Files** — Lists all 5 output files
8. **Guidelines and CI** — Points to CLAUDE.md, SKILL.md, lint tools, and GitHub Actions

---

### Root `README.md` — Project Overview

**What it is:** The top-level README that provides the four-phase evolution story of the project.

**Phase 4 section added:**
```markdown
### Phase 4: Multi-Agent Telemetry with Guidelines, MCP, and RAG
* Goal: Strict deterministic boundaries, offline development, and RAG grounding.
* Tech: C11 Core + Python LLM Wrapper + ChromaDB + GitHub Actions.
* Focus:
  * "Rules decide → LLM explains": All logic moved to memory-safe C binaries.
  * RAG: LLMs query local documentation via ChromaDB to ground explanations.
  * Tooling: .clang-format, .pylintrc, and GitHub Actions CI.
  * Offline Mode: Mock LLM Mode runs without API keys.
```

**Quick-start command block:**
```bash
cd Phase4
pip install -r requirements.txt
python rag/rag_setup.py
make        # or build.bat on Windows
python main.py
```

---

## 10. Verified Output Files

These files are generated by the pipeline and exist in `outputs/` from the last successful run:

### `outputs/validated_data.json`
Generated by `data_agent.c`. Contains all 21 valid rows (rows 2–8, 11–17, 19–25 of the CSV) grouped by vehicle. Each row is a JSON object with all 12 fields. The validation summary shows `valid_rows: 21, skipped_rows: 0`.

### `outputs/analysis_results.json`
Generated by `analysis_agent.c`. Contains per-vehicle KPIs and anomaly lists for all 10 metrics. The anomaly arrays reflect the union of 3-sigma and domain threshold violations.

### `outputs/fleet_status.json`
Generated by `classification_agent.c`. The ground truth classification result:
```json
{
  "CAR_A": {"status": "Healthy",   "total_anomalies": 2,  "metrics_affected": 1},
  "CAR_B": {"status": "Critical",  "total_anomalies": 8,  "metrics_affected": 7},
  "CAR_C": {"status": "Critical",  "total_anomalies": 44, "metrics_affected": 9}
}
```

### `outputs/final_report.md`
Generated by `formatting_agent.py` (GPT-4o). A markdown fleet report with an executive summary and per-vehicle sections containing KPIs, health status, and the Opus-generated insights.

### `outputs/compliance_report.txt`
Generated by `coding_agent.py` (GPT-4o-mini). A compliance verdict starting with `"COMPLIANT:"` or `"NON-COMPLIANT:"` with justification. The verified output contains: `COMPLIANT: The report adheres to all Phase 4 guidelines. No hallucinations or non-deterministic logic found in the core pipeline.`

---

## 11. End-to-End Data Flow

This section traces how a single row of data — `2024-01-01 00:03:00, CAR_B, 128, 840, 0.31, ...` — flows through every layer:

### CSV → data_agent

`data_agent.c` reads the row. The `is_number` check confirms `128` is numeric. It is stored as `cJSON_AddNumberToObject(row_obj, "temperature", 128.0)` in `data["CAR_B"]` array.

### validated_data.json → analysis_agent

`analysis_agent.c` collects all 7 CAR_B temperature values: `[92, 95, 100, 128, 120, 108, 104]`. It computes:
- `mean = 106.7, std = 13.7`
- 3-sigma upper bound: `106.7 + 3×13.7 = 147.8`
- `128` is NOT beyond 147.8, so NO statistical anomaly
- Domain check: `128 > 120` → YES, domain anomaly → `128.0` added to anomaly array

### analysis_results.json → classification_agent

`classification_agent.c` sums all anomaly counts across all CAR_B metrics. The result: `total_anomalies = 8, metrics_affected = 7`. Rule check: `metrics_affected >= 7` → **Critical**.

### fleet_status.json → search_agent

`retrieve_context_for_vehicle("CAR_B", {"temperature": [128.0], "latency": [840.0], ...})` → Sonnet generates queries like `"temperature threshold 120 degrees"` and `"latency threshold 800ms"` → ChromaDB returns chunks from `thresholds.md`.

### RAG context → explanation_agent

`generate_vehicle_insight("CAR_B", {"status": "Critical", ...}, kpis, anomalies, rag_context)` → Opus receives the full context including the RAG-retrieved threshold definitions and generates:
```
1) What is happening
   CAR_B experienced a temperature spike to 128°C (threshold: 120°C) and 
   latency exceeding 840ms, triggering Critical status across 7 metrics.
2) Likely causes
   - Cooling system degradation
   - Communication hardware stress
   - ...
3) Recommended next actions
   - Schedule immediate inspection
   - ...
```

### insights + fleet_status → formatting_agent

GPT-4o compiles everything into a professional markdown report with headers, executive summary, and per-vehicle sections.

### final_report.md → coding_agent

GPT-4o-mini reads CLAUDE.md and the report, confirms that the status was presented as a deterministic fact (not an LLM guess), and returns `COMPLIANT:`.

---

## Requirements Coverage Summary

| Requirement | Implementation |
|-------------|---------------|
| **Req 1: Three-Layer Guidelines** | Layer 1: `CLAUDE.md`; Layer 2: `skills/SKILL.md`; Layer 3: `.clang-format`, `.pylintrc`, `.pre-commit-config.yaml`, `.github/workflows/lint.yml` |
| **Req 2: GitHub MCP** | `mcp-config.json` — Cursor-side config, GitHub token from `.env`, three intended uses documented |
| **Req 3: RAG (ChromaDB + HuggingFace)** | `rag_setup.py` (index builder), `rag_query.py` (query interface), `docs/` (source documents), `vector_store/` (persistence) |
| **Req 4a: C Agents** | `data_agent.c`, `analysis_agent.c`, `classification_agent.c`, all porting Phase 3 logic exactly |
| **Req 4b: Python Agents** | `search_agent.py` (Sonnet), `explanation_agent.py` (Opus), `formatting_agent.py` (GPT-4o), `coding_agent.py` (GPT-4o-mini) |
| **Req 4c: Orchestrator** | `main.py` — 7-step pipeline, C via subprocess IPC, Python agents as function calls |
| **C parity verification** | `fleet_status.json` matches Phase 3 Python output: CAR_A=Healthy, CAR_B=Critical, CAR_C=Critical |
| **Offline / no-API-keys mode** | `llm_client.py` mock mode — entire pipeline runs and produces outputs with zero API keys |
| **Memory safety** | Every `malloc` → `free`, every `cJSON_Parse` → `cJSON_Delete`, no `gets()` or `sprintf()` |
| **JSON IPC contract** | stdin/stdout only, no shared memory, no temp files, each agent validates input before processing |
| **Windows support** | `build.bat` for MinGW, path separator handling in `main.py`, `.exe` detection logic |
| **Root README updated** | Phase 4 section with quick-start and tech stack description |
