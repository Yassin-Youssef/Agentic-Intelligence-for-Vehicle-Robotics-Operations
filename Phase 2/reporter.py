#report layer responsible for turning analysis results into readable insight
from typing import Dict, Any
class ReporterAgent:
    def generate_report(self,analysis_results: Dict[str, Any]) -> str:
        """Args: analysis_results Dict[str, Any]: the output from analyze_metrics
        Returns: str formatted report
        for phase 2 behaviour with for metrics and seperate vehicle analysis"""
        report_lines = []#store each line of the report as a list
        report_lines = []  # stores each line of the report as a list of strings

        # Report header
        report_lines.append("Operational Metrics Report (Phase 2 Fleet Analysis)\n")
        
        #phase 2 with seperate vehicles
        for vehicle_id, vehicle_results in analysis_results.items():
            report_lines.append("=" * 30)
            report_lines.append(f"Vehicle ID: {vehicle_id}")
            report_lines.append("=" * 30)
            # Extract KPIs and anomalies for THIS vehicle
            kpis = vehicle_results.get("kpis", {})
            anomalies = vehicle_results.get("anomalies", {})
            # Loop over each metric for the current vehicle
            for metric, values in kpis.items():
                report_lines.append(f"\nMetric: {metric}")
                report_lines.append(f"- Mean: {values['mean']:.2f}")
                report_lines.append(f"- Min: {values['min']:.2f}")
                report_lines.append(f"- Max: {values['max']:.2f}")
                report_lines.append(f"- Std Dev: {values['std']:.2f}")
                # Count how many anomalies were detected for this metric
                anomaly_count = len(anomalies.get(metric, []))
                if anomaly_count > 0:
                    report_lines.append(f"- Anomalies detected: {anomaly_count}")
                else:
                    report_lines.append("- No anomalies detected")
            report_lines.append("")  # spacing between vehicles
        return "\n".join(report_lines)