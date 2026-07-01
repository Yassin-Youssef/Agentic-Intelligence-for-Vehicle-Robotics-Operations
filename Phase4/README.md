# Phase 4 — Multi-Agent Telemetry Pipeline

Phase 4 represents the final architectural iteration of the Vehicle Robotics Operations pipeline. It strictly enforces the principle of **"Rules decide → LLM explains"**.

## Architecture

The pipeline uses 7 agents across two distinct technology layers, communicating via JSON IPC.

### 1. Deterministic Core (C11)
All decision-making, threshold evaluation, and data processing occur in memory-safe, compiled C binaries. No API keys are required.
* **`data_agent`**: Parses CSV telemetry and validates schemas.
* **`analysis_agent`**: Computes KPIs and detects statistical (3-sigma) and domain anomalies.
* **`classification_agent`**: Classifies vehicles as Healthy, Warning, or Critical based on strict anomaly counts.

### 2. Intelligence Layer (Python + RAG)
LLMs are restricted to explanation and formatting, grounded by a local ChromaDB RAG index containing system documentation.
* **`search_agent` (Claude Sonnet)**: Retrieves relevant thresholds from `docs/` via RAG.
* **`explanation_agent` (Claude Opus)**: Generates human-readable technical insights explaining *why* the C layer classified the vehicle the way it did.
* **`formatting_agent` (GPT-4o)**: Compiles all data into a cohesive Markdown report.
* **`coding_agent` (GPT-4o-mini)**: Audits the final report to ensure the LLMs didn't hallucinate rules or violate the deterministic architecture.

## Getting Started

### Prerequisites
* Python 3.12+
* GCC or Clang (MinGW on Windows)
* Make (optional on Windows, use `build.bat`)

### Installation & Setup

1. **Install Python dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

2. **Build the RAG Index:**
   ```bash
   python rag/rag_setup.py
   ```

3. **Compile the C Agents:**
   ```bash
   make          # Unix/macOS/Linux
   build.bat     # Windows
   ```

### Running the Pipeline

You can run the full orchestration pipeline with:
```bash
python main.py
```

**Note on API Keys:** The pipeline is designed for offline development. If `ANTHROPIC_API_KEY` and `OPENAI_API_KEY` are not set in the `.env` file, the `llm_client.py` will automatically engage **Mock Mode**, providing deterministic dummy responses that simulate the LLM outputs without requiring network requests.

### Output Files
All artifacts are saved to `outputs/`:
* `validated_data.json`
* `analysis_results.json`
* `fleet_status.json`
* `final_report.md`
* `compliance_report.txt`

## Guidelines and CI
This module enforces rigorous quality control:
* `CLAUDE.md`: Mandatory architectural rules.
* `skills/SKILL.md`: Self-check playbooks.
* Local formatting: `.clang-format`, `.pylintrc`, and `pre-commit` hooks.
* GitHub Actions: `pipeline.yml` runs standalone tests and mock orchestrations on every push.
