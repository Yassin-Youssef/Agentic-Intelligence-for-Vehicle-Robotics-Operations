#the analyze layer, responsible for analyzing operational metrics data, does not load or generate
from typing import Dict, Any
import pandas as pd
def analyze_metrics(data: pd.DataFrame) -> Dict[str, Any]:
    """"analyze operational metrics and extract KPIs and anomalies
    Args: data (pd.DataFrame): Operational metrics data
    Returns: Dict[str, Any] dictionary contaning KPIs and anomalies
    """
    numeric_data = data.select_dtypes(include= "number") #selecting only numerical (metric) coloumns
    if numeric_data.empty:
        raise ValueError("No numeric metrics found in the datset")
    analysis_results= { #the container for results
        "kpis":{},
        "anomalies":{},
    }
    for column in numeric_data.columns:
        series = numeric_data[column]
        analysis_results["kpis"][column] = {
            "mean": series.mean(),
            "min": series.min(),
            "max": series.max(),
            "std": series.std(),
        }
        # Statistical anomalies (3-sigma rule)
        statistical_anomalies = pd.Series(dtype=float)
        if series.std() > 0:
            statistical_anomalies = series[
                (series > series.mean() + 3 * series.std()) |
                (series < series.mean() - 3 * series.std())
            ]

        #Rule-based anomalies (domain thresholds)
        rule_anomalies = pd.Series(dtype=float)

        if column == "temperature":
            rule_anomalies = series[series > 120]

        elif column == "latency":
            rule_anomalies = series[series > 800]

        elif column == "error_rate":
            rule_anomalies = series[series > 0.3]

        # Combine both anomaly sources 
        anomalies = pd.concat(
            [statistical_anomalies, rule_anomalies]
        ).drop_duplicates()

        analysis_results["anomalies"][column] = anomalies.tolist()


    return analysis_results