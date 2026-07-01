#the analyze layer, responsible for analyzing operational metrics data, does not load or generate
from typing import Dict, Any
import pandas as pd
def analyze_metrics(data: pd.DataFrame) -> Dict[str, Any]:
    """"analyze operational metrics and extract KPIs and anomalies
    Args: data (pd.DataFrame): Operational metrics data
    Returns: Dict[str, Any] dictionary contaning KPIs and anomalies
    """
    if "vehicle_id" not in data.columns:#each row belong to specfic vehicle
        raise ValueError("Phase 2 requires vehicle_id column")
    analysis_results = {}#container for all vehicles
    #fleet level processing, group telemtry by vehicle id so each vehicle independently analyzed
    for vehicle_id, vehicle_data in data.groupby("vehicle_id"):
        numeric_data = vehicle_data.select_dtypes(include="number")
        if numeric_data.empty:#if no numeric value we skip
            continue
        analysis_results[vehicle_id] = {#initializing container 
            "kpis": {},
            "anomalies": {}
        }
        for column in numeric_data.columns:#loop through each numeric metric for this vehicle
            series = numeric_data[column]
            analysis_results[vehicle_id]["kpis"][column] = {#basic statistical KPIs for the metric
                "mean": series.mean(),
                "min": series.min(),
                "max": series.max(),
                "std": series.std(),
            }
            #Statistical anomalies (3-sigma rule)
            statistical_anomalies = pd.Series(dtype=float)
            if series.std() > 0:#avoiding division by 0
                statistical_anomalies = series[
                    (series > series.mean() + 3 * series.std()) |
                    (series < series.mean() - 3 * series.std())
                ]

            #Rule-based anomalies (domain thresholds)
            rule_anomalies = pd.Series(dtype=float)

            if column == "temperature":#overheating threshold
                rule_anomalies = series[series > 120]

            elif column == "latency":#time delay between a datta signal being sent (ms)
                rule_anomalies = series[series > 800]

            elif column == "error_rate":#error rate beyond acceptable
                rule_anomalies = series[series > 0.3]
        
            elif column == "battery_voltage":#voltage outside safe operating
                rule_anomalies = series[(series < 11.5) | (series > 13.0)]
        
            elif column == "cpu_usage":# cpu saturation
                rule_anomalies = series[series > 90]
        
            elif column == "memory_usage":#memory pressure threshold
                rule_anomalies = series[series> 85]
        
            elif column == "gps_accuracy":#localization accuracy
                rule_anomalies = series[series > 5.0]
        
            elif column == "network_strength":#weak signal strength dBm
                rule_anomalies = series [series <= -90]
        
            elif column == "vibration":#mechanical vibration indicating wear
                rule_anomalies = series [series > 2.0]
        
            elif column == "wheel_speed_variance":#high variance may indicate traction
                rule_anomalies = series[series > 15]
            # Combine both anomaly sources 
            anomalies = pd.concat(#removes duplicate
                [statistical_anomalies, rule_anomalies]
            ).drop_duplicates()
            #store anomalies for this metric and vehicle
            analysis_results[vehicle_id]["anomalies"][column] = anomalies.tolist()
    return analysis_results