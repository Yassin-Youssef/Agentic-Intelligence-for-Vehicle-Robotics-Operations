#we import typing utilities so we can describe what kind of data the function recieves and returns
from typing import Dict, Any
def classify_vehicle(vehicle_results: Dict[str, Any]) -> Dict[str, Any]:
    """Determining the health status of the vehicle based on the detected anomalies
    its deterministic meaning still no llm implemented
    """
    #extracting the anomalies for this vehicle
    anomalies = vehicle_results.get("anomalies", {})
    #counting the anomalies, for each metric calculate how many anomalie exist
    total_anomalies = sum(len(values)for values in anomalies.values())
    #the affected metrics, meaning ignoring any metrics that has no anomalies
    metrics_affected = sum(1 for values in anomalies.values()if len(values)>0)
    #the vehicle status classification
    #for critical
    if total_anomalies >=20 or metrics_affected >=7 :
        status = "Critical"
    #for warning
    elif total_anomalies >= 5 or metrics_affected >=3:
        status ="Warning"
    #for healthy
    else: 
        status = "Healthy"
    #return structurred result
    return{
        "status": status, #final label
        "total_anomalies": total_anomalies, #raw anomaly count
        "metrics_affected": metrics_affected, #spread of issues
    }