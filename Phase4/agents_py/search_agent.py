"""
search_agent.py — RAG Search Agent

Model: Claude Sonnet 4.6 (claude-sonnet-4-6)
Role: Formulates search queries based on vehicle anomalies and
      retrieves relevant documentation chunks from the RAG index.
"""

from .llm_client import call_llm
from rag.rag_query import query_rag_with_metadata


def retrieve_context_for_vehicle(vehicle_id: str, anomalies: dict) -> str:
    """Retrieve RAG context for a vehicle's specific anomalies.
    
    Args:
        vehicle_id: The vehicle identifier.
        anomalies: Dictionary of metric names to lists of anomalous values.
        
    Returns:
        Formatted string containing relevant documentation chunks.
    """
    if not anomalies:
        return "No anomalies detected. No specific context required."
        
    # Ask Sonnet to generate search queries
    metrics_list = ", ".join(anomalies.keys())
    
    prompt = f"""You are a search assistant for vehicle telemetry.
The vehicle {vehicle_id} has anomalies in the following metrics: {metrics_list}

Extract exactly 2 short search queries to find the relevant thresholds and rules 
for these metrics in our documentation.
Format your response as a comma-separated list of queries, nothing else.
"""
    
    response = call_llm(
        prompt=prompt,
        provider="anthropic",
        model="claude-sonnet-4-6",
        system_prompt="You are a helpful search assistant. Output ONLY a comma-separated list of queries."
    )
    
    # Parse queries and execute RAG search
    queries = [q.strip() for q in response.split(",") if q.strip()]
    if not queries:
        # Fallback if LLM fails to format correctly
        queries = [f"{metric} threshold" for metric in anomalies.keys()][:2]
        
    all_chunks = []
    seen_texts = set()
    
    for query in queries:
        results = query_rag_with_metadata(query, top_k=2)
        for r in results:
            if r["text"] not in seen_texts:
                seen_texts.add(r["text"])
                all_chunks.append(f"[Source: {r['source']}]\n{r['text']}")
                
    if not all_chunks:
        return "No specific documentation found in RAG index."
        
    return "\n\n".join(all_chunks)
