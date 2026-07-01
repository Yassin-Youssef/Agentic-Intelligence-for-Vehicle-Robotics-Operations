"""
main.py — Phase 4 Orchestrator

Executes the end-to-end multi-agent telemetry pipeline.

Architecture Pipeline:
1. C Data Agent (CSV -> JSON)
2. C Analysis Agent (KPIs + Anomalies)
3. C Classification Agent (Health Status)
4. Python Search Agent (RAG Context)
5. Python Explanation Agent (LLM Insights)
6. Python Formatting Agent (Markdown Report)
7. Python Coding Agent (Compliance Validation)
"""

import os
import sys
import json
import subprocess
from dotenv import load_dotenv

# Import Python Agents
from agents_py.search_agent import retrieve_context_for_vehicle
from agents_py.explanation_agent import generate_vehicle_insight
from agents_py.formatting_agent import format_final_report
from agents_py.coding_agent import validate_report


def run_c_agent(executable_path: str, input_arg: str = None, input_data: str = None) -> dict:
    """Run a C agent binary and return its parsed JSON output.
    
    Args:
        executable_path: Path to the compiled C binary.
        input_arg: Optional file path argument to pass to the binary.
        input_data: Optional string data to pass via stdin.
        
    Returns:
        Parsed JSON dictionary.
    """
    if not os.path.exists(executable_path) and not executable_path.endswith(".exe"):
        # On Windows, try appending .exe
        if os.path.exists(executable_path + ".exe"):
            executable_path += ".exe"
            
    if not os.path.exists(executable_path):
        print(f"ERROR: C agent binary not found at {executable_path}")
        print("Please run 'build.bat' or 'make' first.")
        sys.exit(1)

    cmd = [executable_path]
    if input_arg:
        cmd.append(input_arg)
        
    try:
        if input_data:
            result = subprocess.run(
                cmd, 
                input=input_data, 
                text=True, 
                capture_output=True, 
                check=True
            )
        else:
            result = subprocess.run(
                cmd, 
                text=True, 
                capture_output=True, 
                check=True
            )
            
        try:
            return json.loads(result.stdout)
        except json.JSONDecodeError as e:
            print(f"ERROR: Failed to parse JSON from {executable_path}")
            print(f"Output was: {result.stdout}")
            sys.exit(1)
            
    except subprocess.CalledProcessError as e:
        print(f"ERROR: C agent {executable_path} failed with exit code {e.returncode}")
        print(f"Stderr: {e.stderr}")
        sys.exit(1)


def main():
    # Load environment variables (API keys)
    load_dotenv()
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    csv_path = os.path.join(script_dir, "data", "sample_metrics.csv")
    outputs_dir = os.path.join(script_dir, "outputs")
    c_agents_dir = os.path.join(script_dir, "agents_c")
    
    os.makedirs(outputs_dir, exist_ok=True)
    
    print("=== Phase 4 Multi-Agent Telemetry Pipeline ===")
    
    # ---------------------------------------------------------
    # LAYER 1: DETERMINISTIC C AGENTS
    # ---------------------------------------------------------
    print("\n[1/7] Running Data Agent (C)...")
    data_agent_exe = os.path.join(c_agents_dir, "data_agent")
    validated_data = run_c_agent(data_agent_exe, input_arg=csv_path)
    with open(os.path.join(outputs_dir, "validated_data.json"), "w") as f:
        json.dump(validated_data, f, indent=2)
        
    print("[2/7] Running Analysis Agent (C)...")
    analysis_agent_exe = os.path.join(c_agents_dir, "analysis_agent")
    # Pass data via stdin (IPC)
    analysis_results = run_c_agent(analysis_agent_exe, input_data=json.dumps(validated_data))
    with open(os.path.join(outputs_dir, "analysis_results.json"), "w") as f:
        json.dump(analysis_results, f, indent=2)
        
    print("[3/7] Running Classification Agent (C)...")
    classification_agent_exe = os.path.join(c_agents_dir, "classification_agent")
    # Pass data via stdin (IPC)
    fleet_status = run_c_agent(classification_agent_exe, input_data=json.dumps(analysis_results))
    with open(os.path.join(outputs_dir, "fleet_status.json"), "w") as f:
        json.dump(fleet_status, f, indent=2)
        
    # ---------------------------------------------------------
    # LAYER 2: LLM AGENTS (INTELLIGENCE)
    # ---------------------------------------------------------
    vehicles = validated_data.get("vehicles", [])
    all_insights = {}
    
    print("\n[4/7] Running Search Agent (Sonnet + RAG) per vehicle...")
    print("[5/7] Running Explanation Agent (Opus) per vehicle...")
    results_dict = analysis_results.get("results", {})
    status_dict = fleet_status.get("fleet_status", {})

    for vid in vehicles:
        print(f"  -> Processing {vid}...")
        v_results = results_dict.get(vid, {})
        v_status = status_dict.get(vid, {})

        kpis = v_results.get("kpis", {})
        anomalies = v_results.get("anomalies", {})

        # Step 4: Search Agent (Sonnet + RAG)
        rag_context = retrieve_context_for_vehicle(vid, anomalies)

        # Step 5: Explanation Agent (Opus)
        insight = generate_vehicle_insight(
            vehicle_id=vid,
            status_info=v_status,
            kpis=kpis,
            anomalies=anomalies,
            rag_context=rag_context
        )
        all_insights[vid] = insight

    # Step 6: Formatting Agent (GPT-4o)
    print("\n[6/7] Running Formatting Agent (GPT-4o)...")
    final_report = format_final_report(fleet_status, analysis_results, all_insights)
    report_path = os.path.join(outputs_dir, "final_report.md")
    with open(report_path, "w", encoding="utf-8") as f:
        f.write(final_report)
    print(f"  -> Report saved to {report_path}")
    
    # Step 7: Coding Agent (GPT-4o-mini Guidelines Validation)
    print("\n[7/7] Running Coding Agent (GPT-4o-mini Compliance Validation)...")
    validation_result = validate_report(final_report)
    compliance_path = os.path.join(outputs_dir, "compliance_report.txt")
    with open(compliance_path, "w", encoding="utf-8") as f:
        f.write(validation_result)
    print(f"  -> Compliance result saved to {compliance_path}")
    
    print("\n=== Pipeline Complete ===")
    print(f"Validation Verdict: {validation_result.splitlines()[0]}")


if __name__ == "__main__":
    main()
