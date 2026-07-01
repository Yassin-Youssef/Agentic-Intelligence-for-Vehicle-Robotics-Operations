"""
Report schema definitions for the fleet operational monitoring system.

This module documents the expected structure of the generated fleet report.
It can later be used for validation, serialization, or API integration.
"""

from typing import Dict, TypedDict


class MetricSummary(TypedDict):
    mean: float
    min: float
    max: float
    std: float
    anomalies: int


class VehicleReport(TypedDict):
    vehicle_id: str
    health_status: str
    metrics: Dict[str, MetricSummary]


class FleetReport(TypedDict):
    vehicles: Dict[str, VehicleReport]
