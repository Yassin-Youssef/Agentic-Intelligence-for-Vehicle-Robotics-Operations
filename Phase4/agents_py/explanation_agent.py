"""
explanation_agent.py — Telemetry Explanation Agent

Model: Claude Opus 4.8 (claude-opus-4-8)
Role: Generates human-readable insights for a vehicle based on 
      deterministic KPIs, health status, and RAG-provided context.
      Strictly adheres to the "Rules decide -> LLM explains" constraint.
"""

from .llm_client import call_llm


def generate_vehicle_insight(vehicle_id: str, status_info: dict, kpis: dict, anomalies: dict, rag_context: str) -> str:
    """Generate technical insights explaining the vehicle's state.
    
    Args:
        vehicle_id: The vehicle identifier.
        status_info: Health status dict (status, total_anomalies, metrics_affected).
        kpis: KPI dict from analysis agent.
        anomalies: Anomalies dict from analysis agent.
        rag_context: Formatted string of relevant documentation from the search agent.
        
    Returns:
        A concise, markdown-formatted technical insight string.
    """
    status = status_info.get("status", "Unknown")
    total = status_info.get("total_anomalies", 0)
    affected = status_info.get("metrics_affected", 0)
    
    # Build anomaly summary
    anomaly_lines = []
    for metric, values in anomalies.items():
        if values:
            anomaly_lines.append(f"- {metric}: {len(values)} anomalies detected")
            
    if not anomaly_lines:
        anomaly_lines.append("- No anomalies detected")
        
    # Build KPI summary
    kpi_lines = []
    for metric, kpi in kpis.items():
        if metric in anomalies and anomalies[metric]:
            kpi_lines.append(f"- {metric}: mean={kpi['mean']:.2f}, max={kpi['max']:.2f}")
            
    if not kpi_lines:
        kpi_lines.append("No specific KPIs highlighted.")
        
    prompt = f"""You are an automotive/robotics operations engineer.
Write a concise technical insight for this vehicle.

IMPORTANT CONSTRAINT (CLAUDE.md): You must NOT classify the vehicle or alter the metrics.
Your ONLY job is to EXPLAIN the deterministic results provided below, grounding your
explanation in the provided documentation context.

--- VEHICLE DATA ---
Vehicle: {vehicle_id}
Deterministic Status: {status}
Total Anomalies: {total}
Metrics Affected: {affected}

Anomaly Summary:
{chr(10).join(anomaly_lines)}

KPI Highlights (for anomalous metrics):
{chr(10).join(kpi_lines)}

--- DOCUMENTATION CONTEXT (RAG) ---
{rag_context}

--- INSTRUCTIONS ---
Output a technical insight in exactly this format:
1) What is happening (1-3 sentences)
2) Likely causes (bullet list, max 3)
3) Recommended next actions (bullet list, max 3)

Be conservative. Do NOT invent sensors or data we do not have.
"""

    return call_llm(
        prompt=prompt,
        provider="anthropic",
        model="claude-opus-4-8",
        system_prompt="You are a strict, factual telemetry engineer. You only explain data; you do not invent facts."
    )
