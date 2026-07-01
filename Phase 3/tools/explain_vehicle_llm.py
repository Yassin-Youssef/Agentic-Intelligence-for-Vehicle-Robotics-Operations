#this file is responsible for the llm based reasoning, turns raw kpis and anomalies to human readable insight
from typing import Dict, Any
import os #reading environment variables
from dotenv import load_dotenv #loads the env file into envrionment variable
from openai import OpenAI #openAi sdk
load_dotenv() # loads variables from env so getenv can read them
OPENROUTER_API_KEY = os.getenv("OPENROUTER_API_KEY")#reads open router api key
OPENROUTER_MODEL = os.getenv("OPENROUTER_MODEL", "openai/gpt-4o-mini")#choose model to enter
#saffte check if fail
if not OPENROUTER_API_KEY:
    raise ValueError("Missing OPENROUTER_API_KEY. put the env file")
#creating a client using openrouter openai compatible 
client = OpenAI(api_key=OPENROUTER_API_KEY, base_url="https://openrouter.ai/api/v1")
def explain_vehicle_llm(vehicle_id: str, vehicle_results: Dict[str, Any], status_info: Dict[str, Any]) -> str:
    """generate a short techinical report expalanation for one vehicle using llm
     Args: vehicle_id: vehicle identifier
     vehicle_results: output for that vehicle from analyze_metric
     status_info: output from that vehicle from classify_vehicle
     
     returns: a concise natural-language insight string"""
    #pull kpis and anomalies for the vehicle
    kpis = vehicle_results.get("kpis", {})
    anomalies = vehicle_results.get("anomalies", {})
    total_anomalies = status_info.get("total_anomalies", 0)
    metrics_affected = status_info.get("metrics_affected", 0)
    status = status_info.get("status", "Unknown")
    #build a compact anomaly summery
    anomaly_summary_lines = [] #collect readable lines 
    for metric, values in anomalies.items():
        count = len(values)#how many data points were flagged
        if count > 0:
            anomaly_summary_lines.append(f"- {metric}: {count} anomalies")
    if not anomaly_summary_lines:
        anomaly_summary_lines.append("- No anomalies detected")

    #including ome kpi hghlight to give llm context
    kpi_highlights = []
    for metric_line in anomaly_summary_lines:
        if "No anomalies detected" in metric_line:
            continue
        metric = metric_line.split(":")[0].replace("- ", "").strip()  # Extract metric name from the line
        if metric in kpis:
            v = kpis[metric]  # KPI dict for that metric
            kpi_highlights.append(
                f"- {metric}: mean={v['mean']:.2f}, min={v['min']:.2f}, max={v['max']:.2f}, std={v['std']:.2f}"
            )
    if not kpi_highlights:
        kpi_highlights.append("No Kpis highlights")
    #creaing prompt
    prompt = f"""You are an automotive/robotics operations engineer.
Write a concise technical insight for this vehicle using the provided telemetry summary.
Vehicle: {vehicle_id}
Health status: {status}
Total anomalies: {total_anomalies}
Metrics affected: {metrics_affected}
Anomaly summary:
{chr(10).join(anomaly_summary_lines)}
KPI highlights (only for anomalous metrics):
{chr(10).join(kpi_highlights)}
Output format:
1) What is happening (1-3 sentences)
2) Likely causes (bullet list, max 3)
3) Recommended next actions (bullet list, max 3-6 depends on the issues)
Be conservative: do NOT invent sensors or data we do not have.
"""
    #calling the open router via openai compatible chat completions style
    respone = client.chat.completions.create(model=OPENROUTER_MODEL,
       messages=[{"role": "user", "content": prompt}],
       temperature = 0.2, #low temp
    )
    return respone.choices[0].message.content.strip()