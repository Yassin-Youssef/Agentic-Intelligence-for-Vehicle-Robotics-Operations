# Agentic Intelligence for Vehicle Robotics Operations

Welcome to the comprehensive multi-phase project designed to evolve a vehicle telemetry processing pipeline from a simple script into a robust, multi-agent, deterministic architecture.

This project demonstrates how to effectively integrate Large Language Models (LLMs) into critical infrastructure while avoiding hallucinations and ensuring strict, rule-based decision making.

## The Evolution Phases

The project is structured into four distinct phases, each building upon the previous to achieve higher reliability and sophistication.

### [Phase 1: Basic Telemetry Processing](./Phase%201/README.md)
* **Goal:** Establish a baseline telemetry script.
* **Tech:** Pure Python.
* **Focus:** Data ingestion, basic anomaly detection, and JSON output generation.

### [Phase 2: LLM Explanations](./Phase%202/README.md)
* **Goal:** Introduce AI to provide human-readable insights.
* **Tech:** Python + OpenAI.
* **Focus:** Passing raw telemetry to an LLM to generate insights. Demonstrates the risks of mixing logic with LLMs (hallucinations, non-deterministic classification).

### [Phase 3: Multi-Agent Specialization](./Phase%203/README.md)
* **Goal:** Separate responsibilities using specialized agents.
* **Tech:** Python + LangChain/Anthropic/OpenAI.
* **Focus:** Breaking the monolithic script into specialized Python tools (`load_data`, `analyze_metrics`, `classify_vehicle`, `explain_vehicle_llm`). Improves reliability but still lacks strict language boundaries.

### [Phase 4: Multi-Agent Telemetry with Guidelines, MCP, and RAG](./Phase4/README.md)
* **Goal:** Strict deterministic boundaries, offline development, and RAG grounding.
* **Tech:** C11 Core + Python LLM Wrapper + ChromaDB + GitHub Actions.
* **Focus:** 
  * **"Rules decide → LLM explains"**: All logic and classification is moved to memory-safe C binaries.
  * **RAG**: LLMs query local documentation (`docs/`) via ChromaDB to ground their explanations.
  * **Tooling**: `.clang-format`, `.pylintrc`, and GitHub Actions CI enforce strict coding standards.
  * **Offline Mode**: The Python pipeline supports a Mock LLM Mode to allow the entire orchestrator to run locally without API keys.

## Getting Started

To see the final, production-ready architecture, navigate to `Phase4/`:

```bash
cd Phase4
pip install -r requirements.txt
python rag/rag_setup.py
make        # or build.bat on Windows
python main.py
```

Read the [Phase 4 README](./Phase4/README.md) for detailed architectural documentation.
