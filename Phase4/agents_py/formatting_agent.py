"""
formatting_agent.py — Report Formatting Agent

Model: GPT-4o (or GPT-4)
Role: Compiles the individual vehicle insights, KPIs, and statuses 
      into a cohesive markdown final report.
"""

from .llm_client import call_llm
import json


def format_final_report(fleet_status: dict, analysis_results: dict, insights: dict) -> str:
    """Format all pipeline data into a final markdown report.
    
    Args:
        fleet_status: The output of the classification agent.
        analysis_results: The output of the analysis agent.
        insights: Dictionary mapping vehicle_id to LLM-generated insight strings.
        
    Returns:
        Formatted markdown report.
    """
    # Create a compact summary for the prompt
    summary_data = []
    
    for vid, status_info in fleet_status.get("fleet_status", {}).items():
        summary_data.append({
            "vehicle": vid,
            "status": status_info["status"],
            "anomalies": status_info["total_anomalies"],
            "insight": insights.get(vid, "No insight generated.")
        })
        
    prompt = f"""You are a technical report formatter for a vehicle robotics fleet.
Format the final fleet status report using the data below.

DATA TO FORMAT:
{json.dumps(summary_data, indent=2)}

INSTRUCTIONS:
1. Start with a "# Phase 4 Fleet Telemetry Report" header.
2. Provide a brief 1-paragraph executive summary of the fleet's overall health.
3. For each vehicle, create a subsection (## Vehicle ID (Status)).
4. Include the total anomalies for that vehicle.
5. Provide the exact insight text provided in the data. Do NOT alter the insight text.
6. Use clean, professional Markdown formatting.
"""

    return call_llm(
        prompt=prompt,
        provider="openai",
        model="gpt-4o",
        system_prompt="You are a precise report formatting agent. You present data beautifully in Markdown without altering facts."
    )
