/*
 * classification_agent.c — Vehicle Health Classification Agent
 *
 * Reads analysis results JSON (from analysis_agent), classifies each
 * vehicle as Healthy, Warning, or Critical based on anomaly counts.
 *
 * Input:  Analysis results JSON (file path argument or stdin)
 * Output: Per-vehicle health status JSON to stdout
 *
 * Port of: Phase 3/tools/classify_vehicle.py
 *
 * Classification Rules:
 *   Critical:  total_anomalies >= 20  OR  metrics_affected >= 7
 *   Warning:   total_anomalies >= 5   OR  metrics_affected >= 3
 *   Healthy:   otherwise
 *
 * JSON Output Schema:
 * {
 *   "status": "success",
 *   "fleet_status": {
 *     "CAR_A": {
 *       "status": "Healthy",
 *       "total_anomalies": 2,
 *       "metrics_affected": 1
 *     },
 *     ...
 *   }
 * }
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_utils.h"

#define MAX_INPUT_SIZE (1024 * 1024)

/* Read entire file or stdin into a buffer */
static char *read_input(const char *filepath) {
    FILE *fp;
    if (filepath != NULL) {
        fp = fopen(filepath, "r");
        if (fp == NULL) return NULL;
    } else {
        fp = stdin;
    }

    char *buffer = (char *)malloc(MAX_INPUT_SIZE);
    if (buffer == NULL) {
        if (filepath != NULL) fclose(fp);
        return NULL;
    }

    size_t total = 0;
    size_t bytes_read;
    while ((bytes_read = fread(buffer + total, 1,
                               MAX_INPUT_SIZE - total - 1, fp)) > 0) {
        total += bytes_read;
        if (total >= MAX_INPUT_SIZE - 1) break;
    }
    buffer[total] = '\0';

    if (filepath != NULL) fclose(fp);
    return buffer;
}

/* Output a JSON error and exit */
static void error_exit(const char *message) {
    cJSON *err = cJSON_CreateObject();
    cJSON_AddStringToObject(err, "status", "error");
    cJSON_AddStringToObject(err, "message", message);
    char *output = cJSON_Print(err);
    if (output != NULL) {
        printf("%s\n", output);
        cJSON_free(output);
    }
    cJSON_Delete(err);
    exit(1);
}

int main(int argc, char *argv[]) {
    /* Read input JSON */
    const char *filepath = (argc >= 2) ? argv[1] : NULL;
    char *input_str = read_input(filepath);
    if (input_str == NULL) {
        error_exit("Failed to read input");
    }

    /* Parse input */
    cJSON *input = cJSON_Parse(input_str);
    free(input_str);
    if (input == NULL) {
        error_exit("Failed to parse input JSON");
    }

    /* Validate input structure */
    cJSON *status_field = cJSON_GetObjectItem(input, "status");
    if (status_field == NULL || !cJSON_IsString(status_field) ||
        strcmp(status_field->valuestring, "success") != 0) {
        cJSON_Delete(input);
        error_exit("Input JSON does not have status=success");
    }

    cJSON *results = cJSON_GetObjectItem(input, "results");
    if (results == NULL || !cJSON_IsObject(results)) {
        cJSON_Delete(input);
        error_exit("Input JSON missing 'results' object");
    }

    /* Build fleet_status object */
    cJSON *fleet_status = cJSON_CreateObject();

    /* Iterate over each vehicle */
    cJSON *vehicle_result;
    cJSON_ArrayForEach(vehicle_result, results) {
        const char *vehicle_id = vehicle_result->string;
        if (vehicle_id == NULL || !cJSON_IsObject(vehicle_result)) {
            continue;
        }

        /* Get anomalies object */
        cJSON *anomalies = cJSON_GetObjectItem(vehicle_result, "anomalies");
        if (anomalies == NULL || !cJSON_IsObject(anomalies)) {
            continue;
        }

        /* Count total anomalies and metrics affected */
        int total_anomalies = 0;
        int metrics_affected = 0;

        cJSON *metric_anomalies;
        cJSON_ArrayForEach(metric_anomalies, anomalies) {
            if (cJSON_IsArray(metric_anomalies)) {
                int count = cJSON_GetArraySize(metric_anomalies);
                total_anomalies += count;
                if (count > 0) {
                    metrics_affected++;
                }
            }
        }

        /* Classification — exact port of Phase 3/tools/classify_vehicle.py */
        const char *health_status;
        if (total_anomalies >= 20 || metrics_affected >= 7) {
            health_status = "Critical";
        } else if (total_anomalies >= 5 || metrics_affected >= 3) {
            health_status = "Warning";
        } else {
            health_status = "Healthy";
        }

        /* Build vehicle status object */
        cJSON *vstatus = cJSON_CreateObject();
        cJSON_AddStringToObject(vstatus, "status", health_status);
        cJSON_AddNumberToObject(vstatus, "total_anomalies", total_anomalies);
        cJSON_AddNumberToObject(vstatus, "metrics_affected", metrics_affected);
        cJSON_AddItemToObject(fleet_status, vehicle_id, vstatus);
    }

    /* Build root output */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "success");
    cJSON_AddItemToObject(root, "fleet_status", fleet_status);

    /* Print and cleanup */
    char *output = cJSON_Print(root);
    if (output != NULL) {
        printf("%s\n", output);
        cJSON_free(output);
    }
    cJSON_Delete(root);
    cJSON_Delete(input);

    return 0;
}
