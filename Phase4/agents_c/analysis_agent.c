/*
 * analysis_agent.c — Metrics Analysis Agent
 *
 * Reads validated vehicle data JSON (from data_agent), computes per-vehicle
 * KPIs (mean, min, max, std) and detects anomalies using:
 *   - 3-sigma statistical rule
 *   - Domain-specific thresholds
 * Combines both anomaly sources with deduplication.
 *
 * Input:  Validated data JSON (file path argument or stdin)
 * Output: Per-vehicle KPIs and anomalies JSON to stdout
 *
 * Port of: Phase 3/tools/analyze_metrics.py
 *
 * JSON Output Schema:
 * {
 *   "status": "success",
 *   "results": {
 *     "CAR_A": {
 *       "kpis": {
 *         "temperature": {"mean": 77.0, "min": 74.0, "max": 80.0, "std": 2.16},
 *         ...
 *       },
 *       "anomalies": {
 *         "temperature": [128.0, ...],
 *         ...
 *       }
 *     },
 *     ...
 *   }
 * }
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "json_utils.h"

#define MAX_INPUT_SIZE (1024 * 1024)  /* 1 MB max input */
#define MAX_VALUES     1024           /* max values per metric per vehicle */

/* ---- Numeric metric names (skip timestamp and vehicle_id) ---- */
static const char *NUMERIC_METRICS[] = {
    "temperature", "latency", "error_rate", "battery_voltage",
    "cpu_usage", "memory_usage", "gps_accuracy", "network_strength",
    "vibration", "wheel_speed_variance",
    NULL
};


/* ---- Statistical functions ---- */

static double compute_mean(const double *values, int count) {
    if (count == 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += values[i];
    }
    return sum / count;
}

static double compute_min(const double *values, int count) {
    if (count == 0) return 0.0;
    double m = values[0];
    for (int i = 1; i < count; i++) {
        if (values[i] < m) m = values[i];
    }
    return m;
}

static double compute_max(const double *values, int count) {
    if (count == 0) return 0.0;
    double m = values[0];
    for (int i = 1; i < count; i++) {
        if (values[i] > m) m = values[i];
    }
    return m;
}

/* Sample standard deviation (ddof=1, matching pandas default) */
static double compute_std(const double *values, int count) {
    if (count <= 1) return 0.0;
    double mean = compute_mean(values, count);
    double sum_sq = 0.0;
    for (int i = 0; i < count; i++) {
        double diff = values[i] - mean;
        sum_sq += diff * diff;
    }
    return sqrt(sum_sq / (count - 1));
}

/* ---- Domain threshold checks ----
 * Returns 1 if the value is anomalous for the given metric.
 * Exact port of Phase 3/tools/analyze_metrics.py thresholds.
 */
static int check_domain_threshold(const char *metric, double value) {
    if (strcmp(metric, "temperature") == 0) {
        return value > 120.0;
    }
    if (strcmp(metric, "latency") == 0) {
        return value > 800.0;
    }
    if (strcmp(metric, "error_rate") == 0) {
        return value > 0.3;
    }
    if (strcmp(metric, "battery_voltage") == 0) {
        return (value < 11.5) || (value > 13.0);
    }
    if (strcmp(metric, "cpu_usage") == 0) {
        return value > 90.0;
    }
    if (strcmp(metric, "memory_usage") == 0) {
        return value > 85.0;
    }
    if (strcmp(metric, "gps_accuracy") == 0) {
        return value > 5.0;
    }
    if (strcmp(metric, "network_strength") == 0) {
        return value <= -90.0;
    }
    if (strcmp(metric, "vibration") == 0) {
        return value > 2.0;
    }
    if (strcmp(metric, "wheel_speed_variance") == 0) {
        return value > 15.0;
    }
    return 0;
}

/* ---- Read entire file or stdin into a buffer ---- */
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

/* Check if value already exists in array (for deduplication) */
static int value_in_array(const double *arr, int count, double val) {
    for (int i = 0; i < count; i++) {
        if (fabs(arr[i] - val) < 1e-10) {
            return 1;
        }
    }
    return 0;
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

    cJSON *data_obj = cJSON_GetObjectItem(input, "data");
    if (data_obj == NULL || !cJSON_IsObject(data_obj)) {
        cJSON_Delete(input);
        error_exit("Input JSON missing 'data' object");
    }

    /* Build results object */
    cJSON *results = cJSON_CreateObject();

    /* Iterate over each vehicle */
    cJSON *vehicle_data;
    cJSON_ArrayForEach(vehicle_data, data_obj) {
        const char *vehicle_id = vehicle_data->string;
        if (vehicle_id == NULL || !cJSON_IsArray(vehicle_data)) {
            continue;
        }

        cJSON *kpis_obj = cJSON_CreateObject();
        cJSON *anomalies_obj = cJSON_CreateObject();

        /* Process each numeric metric */
        for (int m = 0; NUMERIC_METRICS[m] != NULL; m++) {
            const char *metric = NUMERIC_METRICS[m];
            double values[MAX_VALUES];
            int count = 0;

            /* Collect all values for this metric from vehicle rows */
            cJSON *row;
            cJSON_ArrayForEach(row, vehicle_data) {
                cJSON *field = cJSON_GetObjectItem(row, metric);
                if (field != NULL && cJSON_IsNumber(field) &&
                    count < MAX_VALUES) {
                    values[count] = field->valuedouble;
                    count++;
                }
            }

            if (count == 0) {
                continue; /* No data for this metric */
            }

            /* Compute KPIs */
            double mean = compute_mean(values, count);
            double min_val = compute_min(values, count);
            double max_val = compute_max(values, count);
            double std_val = compute_std(values, count);

            cJSON *kpi = cJSON_CreateObject();
            cJSON_AddNumberToObject(kpi, "mean", mean);
            cJSON_AddNumberToObject(kpi, "min", min_val);
            cJSON_AddNumberToObject(kpi, "max", max_val);
            cJSON_AddNumberToObject(kpi, "std", std_val);
            cJSON_AddItemToObject(kpis_obj, metric, kpi);

            /* Detect anomalies */
            double anomaly_values[MAX_VALUES];
            int anomaly_count = 0;

            for (int i = 0; i < count; i++) {
                int is_statistical = 0;
                int is_domain = 0;

                /* 3-sigma rule */
                if (std_val > 0.0) {
                    if (values[i] > mean + 3.0 * std_val ||
                        values[i] < mean - 3.0 * std_val) {
                        is_statistical = 1;
                    }
                }

                /* Domain threshold */
                is_domain = check_domain_threshold(metric, values[i]);

                /* Combine (union) with deduplication */
                if ((is_statistical || is_domain) &&
                    !value_in_array(anomaly_values, anomaly_count,
                                    values[i]) &&
                    anomaly_count < MAX_VALUES) {
                    anomaly_values[anomaly_count] = values[i];
                    anomaly_count++;
                }
            }

            /* Store anomalies as array */
            cJSON *anom_arr = cJSON_CreateArray();
            for (int i = 0; i < anomaly_count; i++) {
                cJSON_AddItemToArray(anom_arr,
                                     cJSON_CreateNumber(anomaly_values[i]));
            }
            cJSON_AddItemToObject(anomalies_obj, metric, anom_arr);
        }

        /* Build vehicle result */
        cJSON *vehicle_result = cJSON_CreateObject();
        cJSON_AddItemToObject(vehicle_result, "kpis", kpis_obj);
        cJSON_AddItemToObject(vehicle_result, "anomalies", anomalies_obj);
        cJSON_AddItemToObject(results, vehicle_id, vehicle_result);
    }

    /* Build root output */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "success");
    cJSON_AddItemToObject(root, "results", results);

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
