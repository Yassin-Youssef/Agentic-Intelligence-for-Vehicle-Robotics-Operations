#report layer responsible for turning analysis results into readable insight
from typing import Dict, Any
class ReporterAgent:
    def generate_report(self,analysis_results: Dict[str, Any]) -> str:
        """Args: analysis_results Dict[str, Any]: the output from analyze_metrics
        Returns: str formatted report"""
        report_lines = []#store each line of the report as a list
        #extracting kpis and anomalies from the analysis dictionary
        kpis = analysis_results.get("kpis", {})
        anomalies = analysis_results.get("anomalies", {})
        report_lines.append("Operational metrics report: \n")#the report header
        #loop for each metric analyzed
        for metric, values in kpis.items():
            report_lines.append(f"Metric: {metric}")
            report_lines.append(f"-Mean: {values['mean']:.2f}")
            report_lines.append(f"-Min: {values['min']:.2f}")
            report_lines.append(f"-Max: {values['max']:.2f}")
            report_lines.append(f"-Std Dev: {values['std']:.2f}")
            anomaly_count = len(anomalies.get(metric,[]))#counting how many anomalies detected
            if anomaly_count > 0:
                report_lines.append(f"Anomalies detected : {anomaly_count}")
            else:
                report_lines.append("No anomalies detected")
            report_lines.append("")
        return "\n".join(report_lines)