# SKILL.md — Phase 4 Self-Check Playbooks

> **Layer 2: Skills** — Task-specific checklists triggered by context.
> Before completing any task, the agent (human or AI) runs the matching playbook
> and confirms every item passes. If any item fails, fix before proceeding.

---

## Playbook A — Writing C Agents

**Trigger:** Creating or modifying any file in `agents_c/`.

### Pre-flight Checks

- [ ] I have read CLAUDE.md Guideline 1 (Separation of Deterministic Logic and LLM Reasoning).
- [ ] I have read CLAUDE.md Guideline 2 (Strict Interface Contracts and Memory Safety).

### Implementation Checks

- [ ] **No LLM calls.** The C agent performs zero HTTP requests, zero API calls.
      All logic is pure computation on input data.
- [ ] **JSON input validation.** The agent checks that all required fields exist in
      the input JSON before processing. Missing or wrong-type fields produce a JSON
      error on stdout and exit code 1.
- [ ] **JSON output schema.** The agent writes valid JSON to stdout that matches the
      documented schema. Output is parseable by Python's `json.loads()`.
- [ ] **Memory safety.** Every `malloc()` / `calloc()` has a matching `free()`.
      Every `cJSON_Parse()` result is checked for NULL before use. Every
      `cJSON_Print()` string is freed with `cJSON_free()`.
- [ ] **No unsafe functions.** No use of `gets()`, `sprintf()`, `strcat()` without
      bounds. Use `fgets()`, `snprintf()`, `strncat()` instead.
- [ ] **Threshold parity.** If modifying `analysis_agent.c`, verify that all 10
      domain thresholds match the values in CLAUDE.md and `docs/thresholds.md`:
      - temperature > 120
      - latency > 800
      - error_rate > 0.3
      - battery_voltage < 11.5 or > 13.0
      - cpu_usage > 90
      - memory_usage > 85
      - gps_accuracy > 5.0
      - network_strength <= -90
      - vibration > 2.0
      - wheel_speed_variance > 15
- [ ] **Classification parity.** If modifying `classification_agent.c`, verify rules
      match CLAUDE.md and `docs/classification_rules.md`:
      - Critical: total_anomalies >= 20 OR metrics_affected >= 7
      - Warning: total_anomalies >= 5 OR metrics_affected >= 3
      - Healthy: otherwise
- [ ] **Builds clean.** `make` (Unix) or `build.bat` (Windows) compiles with zero
      warnings under `-Wall -Wextra`.

### Post-implementation Checks

- [ ] **Output matches Phase 3.** Run the C agent on `data/sample_metrics.csv` and
      compare output with Phase 3 Python tool output. KPI values, anomaly counts,
      and classifications must match exactly (within floating-point tolerance).
- [ ] **Malformed input test.** Feed broken JSON or empty input → agent produces
      error JSON and exits cleanly (no crash, no hang).

---

## Playbook B — Writing Python Agents

**Trigger:** Creating or modifying any file in `agents_py/`.

### Pre-flight Checks

- [ ] I have read CLAUDE.md Guideline 1 (no anomaly re-computation in Python).
- [ ] I have read CLAUDE.md Guideline 2 (standalone functions, env key usage).

### Implementation Checks

- [ ] **No anomaly re-computation.** The Python agent does not compute KPIs, apply
      thresholds, or change classification results. It only reads JSON produced by
      C agents and generates text.
- [ ] **Standalone function.** The agent exposes a single `run()` function. No module-level
      LLM client initialization. The client is created inside `run()` or via a helper
      from `llm_client.py`.
- [ ] **Correct model assignment.** Each agent uses exactly one model:
      - `search_agent.py` → Claude Sonnet (Anthropic)
      - `explanation_agent.py` → Claude Opus (Anthropic)
      - `formatting_agent.py` → GPT (OpenAI)
      - `coding_agent.py` → GPT-4o-mini (OpenAI)
- [ ] **Environment variable usage.** API keys are read from environment variables
      via `os.getenv()` after `load_dotenv()`. No hardcoded keys. Missing key
      produces a clear error message, not a crash.
- [ ] **Prompt structure.** The prompt sent to the LLM:
      - Includes role context ("You are an automotive/robotics operations engineer")
      - Includes only data from C agent JSON outputs (no invented metrics)
      - Specifies output format clearly
      - Sets conservative temperature (≤ 0.3)
- [ ] **Type hints.** All function parameters and return values have type annotations.
- [ ] **Docstrings.** Every public function has a docstring with Args and Returns.
- [ ] **Graceful degradation.** If the API key is missing or the API call fails,
      the agent returns a fallback string (e.g., "Explanation unavailable — API key
      not configured") rather than crashing the pipeline.

### Post-implementation Checks

- [ ] **Dry run.** Call `run()` with sample C agent output and verify the return
      value is a non-empty string with the expected structure.
- [ ] **No side effects.** The function does not write files, modify global state,
      or print to stdout (main.py handles all I/O).

---

## Playbook C — Running the Full Pipeline

**Trigger:** Running `python main.py` or testing the end-to-end pipeline.

### Pre-flight Checks

- [ ] **C agents built.** `make` (Unix) or `build.bat` (Windows) completed successfully.
      All three executables exist: `data_agent`, `analysis_agent`, `classification_agent`.
- [ ] **RAG index exists.** `vector_store/` contains ChromaDB files. If not, run
      `python rag/rag_setup.py` first.
- [ ] **Environment configured.** `.env` file exists with at least one valid API key
      (or accept that Python agents will run in mock mode).

### Execution Checks

- [ ] **Sequential execution.** Pipeline stages run in order:
      1. `data_agent` → `outputs/validated_data.json`
      2. `analysis_agent` → `outputs/analysis_results.json`
      3. `classification_agent` → `outputs/fleet_status.json`
      4. `search_agent` (Sonnet + RAG)
      5. `explanation_agent` (Opus)
      6. `formatting_agent` (GPT) → `outputs/final_report.txt`
      7. `coding_agent` (GPT-4o-mini) → `outputs/compliance_report.txt`
- [ ] **Output files written.** After a successful run, verify all expected output
      files exist in `outputs/`.
- [ ] **C outputs valid.** Each JSON output from C agents parses without errors in Python.
- [ ] **Report completeness.** `final_report.txt` contains sections for every vehicle
      in the input CSV with KPIs, health status, and insights.
- [ ] **Compliance check.** `compliance_report.txt` references CLAUDE.md guidelines
      and reports pass/fail for each rule.

### Failure Recovery

- [ ] **C agent failure.** If a C agent exits with code 1, main.py prints the error
      JSON from stdout and stops the pipeline (do not run Python agents on bad data).
- [ ] **API failure.** If a Python agent's API call fails, main.py logs the error
      and continues with remaining agents (partial report is better than no report).
- [ ] **Missing RAG index.** If `vector_store/` is empty, `search_agent` skips RAG
      and returns empty context. `explanation_agent` still runs with available data.

---

## Playbook D — Modifying Documentation or RAG Sources

**Trigger:** Creating or modifying files in `docs/`.

### Checks

- [ ] **Threshold consistency.** If `docs/thresholds.md` is modified, verify that
      all values still match `analysis_agent.c` and CLAUDE.md.
- [ ] **Classification consistency.** If `docs/classification_rules.md` is modified,
      verify that all rules still match `classification_agent.c` and CLAUDE.md.
- [ ] **RAG rebuild required.** After modifying any file in `docs/`, re-run
      `python rag/rag_setup.py` to rebuild the vector index.
- [ ] **Query test.** After rebuilding, run a test query:
      `python -c "from rag.rag_query import query_rag; print(query_rag('temperature threshold'))"`
      → should return relevant chunk from updated docs.

---

## Playbook E — Lint and CI Changes

**Trigger:** Modifying `.clang-format`, `.pylintrc`, `.pre-commit-config.yaml`, or
`.github/workflows/lint.yml`.

### Checks

- [ ] **Local validation.** Run `pre-commit run --all-files` and verify all hooks pass.
- [ ] **No false positives.** The linting configuration does not flag valid code patterns
      used in the project (e.g., cJSON naming conventions in C).
- [ ] **CI mirrors local.** The checks in `lint.yml` are the same as those in
      `.pre-commit-config.yaml` — no rule runs in CI that doesn't run locally.
