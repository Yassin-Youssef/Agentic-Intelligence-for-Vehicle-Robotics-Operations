from agents.planner import PlannerAgent
from tools.load_data import load_data
from tools.analyze_metrics import analyze_metrics
from agents.reporter import ReporterAgent
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

    #REPORT
    reporter = ReporterAgent()#creating reporter agent
    report = reporter.generate_report(analysis_results)#generate readable report
    print("FINAL REPORT:\n")
    print(report)

    #SAVING REPORT
    output_path = "outputs/final_report.txt"
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(report)
    print(f"\n Report saved to: {output_path}")
if __name__ == "__main__": # ensures main() runs only when this file is excuted
    main()