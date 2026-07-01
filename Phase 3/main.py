from agents.planner import PlannerAgent
from tools.load_data import load_data
from tools.analyze_metrics import analyze_metrics
from agents.reporter import ReporterAgent
from tools.classify_vehicle import classify_vehicle
from tools.explain_vehicle_llm import explain_vehicle_llm
def main () -> None:
    """entry point of the agentic system
    this does and excutes the full pipline:
    DECIDE + SENSE -> ANALYZE -> REPORT"""
    #DECIDE
    planner = PlannerAgent() # create the planner agent
    objective = ( # giving the high level objective shows what system should achieve
        "Analyze the vehicle and robotics operational metrics"
        "and detect potential issues"
    )
    #printing the generated plan for visibility
    plan = planner.create_plan(objective) #asking planner to convert the objective to structured plan
    print("the generated plan is: ")
    print(f"the objective: {plan['objective']}") 
    print("Analysis steps")
    for step in plan["analysis_steps"]:
        print(f"-{step}")
    print("Required tools:")
    for tool in plan["tools_required"]:
        print(f"- {tool}")
    
    #SENSE
    data_path = "data/sample_metrics.csv"#this csv simulates vechile or robot telemetry
    data = load_data(data_path)#load the raw data from CSV file
    
    #ANALYZE
    analysis_results = analyze_metrics(data) #analyzing the loaded data to compute kpis and detect anomalies
    fleet_status = {}#will store status per vehicle
    for vehicle_id, vehicle_results in analysis_results.items():
        fleet_status[vehicle_id] = classify_vehicle(vehicle_results)
    fleet_insights = {} #will store the llm generated explanations per vehicle
    for vehicle_id, vehicle_results in analysis_results.items():
        fleet_insights[vehicle_id] = explain_vehicle_llm(
            vehicle_id=vehicle_id,
            vehicle_results=vehicle_results,
            status_info=fleet_status[vehicle_id]
        )
    print("\nLLM VEHICLE INSIGHTS (Phase 3 – Step 3)\n")
    for vehicle_id, insight in fleet_insights.items():
        print(f"{vehicle_id} AI Insight:\n{insight}\n")
    print("\nVEHICLE HEALTH CLASSIFICATION \n")
    for vehicle_id, status_info in fleet_status.items():
        print(
            f"{vehicle_id}: {status_info['status']},  "
            f"total_anomalies={status_info['total_anomalies']}, "
            f"metrics_affected={status_info['metrics_affected']}"
        )
    #REPORT
    reporter = ReporterAgent()#creating reporter agent
    report = reporter.generate_report(analysis_results, fleet_status)#generate readable report
    print("FINAL REPORT:\n")
    print(report)

    #SAVING REPORT
    output_path = "outputs/final_report.txt"
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(report)
    print(f"\n Report saved to: {output_path}")
if __name__ == "__main__": # ensures main() runs only when this file is excuted
    main()