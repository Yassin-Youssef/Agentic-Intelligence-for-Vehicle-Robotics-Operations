"""
llm_client.py — Universal LLM Interface

Provides a unified interface for calling different LLMs (Anthropic, OpenAI).
Includes a mock mode that returns deterministic responses when no API keys
are provided, fulfilling the "No API keys during dev" constraint.

Usage:
    from llm_client import call_llm
    response = call_llm(prompt="...", provider="anthropic", model="claude-opus-4-8")
"""

import os
from typing import Optional


def call_llm(prompt: str, provider: str, model: str, system_prompt: Optional[str] = None) -> str:
    """Call an LLM, returning a mock response if no API keys are present.

    Mock mode is evaluated at call time (not module load) so that keys set
    after import — e.g. via load_dotenv() in main.py — are respected.

    Args:
        prompt: The user prompt to send.
        provider: "anthropic" or "openai".
        model: Model name (e.g., "claude-sonnet-4-6").
        system_prompt: Optional system instructions.

    Returns:
        The text response from the LLM.
    """
    mock_mode = not os.getenv("ANTHROPIC_API_KEY") and not os.getenv("OPENAI_API_KEY")
    if mock_mode:
        return _mock_llm(prompt, provider, model)
        
    if provider.lower() == "anthropic":
        return _call_anthropic(prompt, model, system_prompt)
    elif provider.lower() == "openai":
        return _call_openai(prompt, model, system_prompt)
    else:
        raise ValueError(f"Unsupported provider: {provider}")


def _call_anthropic(prompt: str, model: str, system_prompt: Optional[str] = None) -> str:
    """Real call to Anthropic API using anthropic package."""
    import anthropic
    client = anthropic.Anthropic(api_key=os.getenv("ANTHROPIC_API_KEY"))
    
    kwargs = {
        "model": model,
        "max_tokens": 1024,
        "temperature": 0.2,
        "messages": [{"role": "user", "content": prompt}]
    }
    if system_prompt:
        kwargs["system"] = system_prompt
        
    response = client.messages.create(**kwargs)
    return response.content[0].text


def _call_openai(prompt: str, model: str, system_prompt: Optional[str] = None) -> str:
    """Real call to OpenAI API using openai package."""
    import openai
    client = openai.OpenAI(api_key=os.getenv("OPENAI_API_KEY"))
    
    messages = []
    if system_prompt:
        messages.append({"role": "system", "content": system_prompt})
    messages.append({"role": "user", "content": prompt})
    
    response = client.chat.completions.create(
        model=model,
        messages=messages,
        temperature=0.2,
        max_tokens=1024,
    )
    return response.choices[0].message.content


def _mock_llm(prompt: str, provider: str, model: str) -> str:
    """Return realistic deterministic responses for development without keys."""
    prompt_lower = prompt.lower()
    
    # Mock responses for Search Agent (Sonnet)
    if "search queries" in prompt_lower or "extract exactly" in prompt_lower:
        if "car_c" in prompt_lower:
            return "cpu usage threshold, network strength normal range"
        return "latency threshold, temperature threshold"
        
    # Mock responses for Explanation Agent (Opus)
    if "write a concise technical insight" in prompt_lower:
        if "car_a" in prompt_lower:
            return (
                "1) What is happening\nThe vehicle is operating normally with minor fluctuations in network strength.\n\n"
                "2) Likely causes\n- Environmental interference\n- Cell tower handoff\n\n"
                "3) Recommended next actions\n- Continue routine monitoring\n- No immediate action required"
            )
        elif "car_b" in prompt_lower:
            return (
                "1) What is happening\nThe vehicle is experiencing significant network drops and latency spikes above 800ms.\n\n"
                "2) Likely causes\n- Failing telemetry antenna\n- Persistent poor coverage area\n\n"
                "3) Recommended next actions\n- Schedule inspection of communication hardware\n- Review route for dead zones"
            )
        else:
            return (
                "1) What is happening\nMultiple critical systems are failing, including severe CPU throttling (>90%) and thermal events (>120C).\n\n"
                "2) Likely causes\n- Cooling system failure\n- Runaway software processes\n- Hardware degradation\n\n"
                "3) Recommended next actions\n- IMMEDIATELY pull vehicle from service\n- Inspect cooling loops\n- Restart control units"
            )
            
    # Mock responses for Formatting Agent (GPT)
    if "format the final fleet" in prompt_lower:
        return (
            "# Phase 4 Telemetry Report\n\n"
            "## CAR_A (Healthy)\nInsights: Operating normally.\n\n"
            "## CAR_B (Warning)\nInsights: Network drops and latency spikes.\n\n"
            "## CAR_C (Critical)\nInsights: Severe CPU throttling and thermal events."
        )
        
    # Mock responses for Coding Agent (GPT-4o-mini)
    if "validate" in prompt_lower or "compliance" in prompt_lower:
        return "COMPLIANT: The report adheres to all Phase 4 guidelines. No hallucinations or non-deterministic logic found in the core pipeline."
        
    return f"[MOCK {provider.upper()} / {model}] Received prompt of length {len(prompt)}"
