# Phase 4 — C Agents Build (Unix/macOS/Linux)
# Requires: gcc (or clang)
#
# Usage:
#   make          — build all agents
#   make clean    — remove compiled binaries
#   make test     — run pipeline on sample data
#
# Each agent is compiled with json_utils.c (vendored cJSON).

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
LDFLAGS = -lm
SRC_DIR = agents_c

AGENTS  = data_agent analysis_agent classification_agent

# Build targets: agents_c/data_agent, agents_c/analysis_agent, etc.
TARGETS = $(addprefix $(SRC_DIR)/, $(AGENTS))

.PHONY: all clean test

all: $(TARGETS)
	@echo "All C agents built successfully."

$(SRC_DIR)/data_agent: $(SRC_DIR)/data_agent.c $(SRC_DIR)/json_utils.c $(SRC_DIR)/json_utils.h
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/data_agent.c $(SRC_DIR)/json_utils.c $(LDFLAGS)

$(SRC_DIR)/analysis_agent: $(SRC_DIR)/analysis_agent.c $(SRC_DIR)/json_utils.c $(SRC_DIR)/json_utils.h
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/analysis_agent.c $(SRC_DIR)/json_utils.c $(LDFLAGS)

$(SRC_DIR)/classification_agent: $(SRC_DIR)/classification_agent.c $(SRC_DIR)/json_utils.c $(SRC_DIR)/json_utils.h
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/classification_agent.c $(SRC_DIR)/json_utils.c $(LDFLAGS)

clean:
	rm -f $(TARGETS)
	rm -f $(SRC_DIR)/*.o
	@echo "Cleaned build artifacts."

test: all
	@echo "=== Running C pipeline on sample data ==="
	$(SRC_DIR)/data_agent data/sample_metrics.csv > outputs/validated_data.json
	@echo "--- data_agent output written to outputs/validated_data.json ---"
	$(SRC_DIR)/analysis_agent outputs/validated_data.json > outputs/analysis_results.json
	@echo "--- analysis_agent output written to outputs/analysis_results.json ---"
	$(SRC_DIR)/classification_agent outputs/analysis_results.json > outputs/fleet_status.json
	@echo "--- classification_agent output written to outputs/fleet_status.json ---"
	@echo "=== C pipeline complete ==="
