#this is the decide layer , it doesnt load or analyze the data
#it outpust the steructured plan
from typing import Dict, List #import type hints to make output readable
class PlannerAgent:#defining the class planner agent is the decide it created a plan witht the steps and tools.
    def create_plan(self, objective: str)-> Dict[str, List[str]]:# methof input objective, out strucrtred plan
        #converting a high level objective into a structured analysis plan
        plan = {
            "objective" : objective, #keeping the original objective
            #the steps include:
            "analysis_steps":[
                "load operational metrics data",#this is reading the logs
                "vqalidate data completness and the consistency",#the sanity checks
                "identify anomilies and/or unsual patterns",#outliers and threshold breachers
                "summerize the key performance indicators (KPIs)",#averages,maxs, error rates, mins, etc
                "genrate insights and recommendations based on the analysis",#turning the analysis into actual steps and fixes
            ],
            "tools_required":[
                "load_data",#live in the tools/load_data
                "analyze_data"#live in the tools/analyze_metrics
            ]
        }
        return plan #retuerning the plan for the main.py