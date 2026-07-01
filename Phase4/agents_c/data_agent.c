/*
 * data_agent.c — Data Loading Agent
 *
 * Reads a CSV file of vehicle telemetry, validates rows, and outputs
 * structured JSON with vehicle list and per-vehicle validated rows.
 *
 * Input:  CSV file path (command-line argument)
 * Output: JSON to stdout
 *
 * Port of: Phase 3/tools/load_data.py
 *
 * JSON Output Schema:
 * {
 *   "status": "success",
 *   "vehicles": ["CAR_A", "CAR_B", ...],
 *   "columns": ["timestamp", "vehicle_id", "temperature", ...],
 *   "data": {
 *     "CAR_A": [
 *       {"timestamp": "...", "temperature": 74.0, ...},
 *       ...
 *     ],
 *     ...
 *   },
 *   "total_rows": 21,
 *   "validation": { "valid_rows": 21, "skipped_rows": 0 }
 * }
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json_utils.h"

#define MAX_LINE_LEN  4096
#define MAX_COLUMNS   64
#define MAX_FIELD_LEN 256

/* Trim leading/trailing whitespace and newlines in-place */
static void trim(char *str) {
    char *start = str;
    char *end;
    while (*start == ' ' || *start == '\t' || *start == '\n' ||
           *start == '\r') {
        start++;
    }
    if (*start == '\0') {
        str[0] = '\0';
        return;
    }
    end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' ||
                           *end == '\r')) {
        end--;
    }
    *(end + 1) = '\0';
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

/* Split a CSV line into fields. Returns number of fields. */
static int split_csv_line(char *line, char fields[][MAX_FIELD_LEN],
                          int max_fields) {
    int count = 0;
    char *ptr = line;
    char *field_start;

    while (*ptr != '\0' && count < max_fields) {
        field_start = ptr;
        while (*ptr != ',' && *ptr != '\n' && *ptr != '\r' && *ptr != '\0') {
            ptr++;
        }
        int len = (int)(ptr - field_start);
        if (len >= MAX_FIELD_LEN) {
            len = MAX_FIELD_LEN - 1;
        }
        strncpy(fields[count], field_start, len);
        fields[count][len] = '\0';
        trim(fields[count]);
        count++;
        if (*ptr == ',') {
            ptr++;
        }
    }
    return count;
}

/* Check if a string is a valid number */
static int is_number(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    if (*str == '-' || *str == '+') {
        str++;
    }
    int has_digit = 0;
    int has_dot = 0;
    while (*str != '\0') {
        if (*str >= '0' && *str <= '9') {
            has_digit = 1;
        } else if (*str == '.' && !has_dot) {
            has_dot = 1;
        } else {
            return 0;
        }
        str++;
    }
    return has_digit;
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
    FILE *fp;
    char line[MAX_LINE_LEN];
    char header_fields[MAX_COLUMNS][MAX_FIELD_LEN];
    char row_fields[MAX_COLUMNS][MAX_FIELD_LEN];
    int num_columns = 0;
    int vehicle_id_col = -1;
    int total_rows = 0;
    int valid_rows = 0;
    int skipped_rows = 0;

    /* Validate arguments */
    if (argc < 2) {
        error_exit("Usage: data_agent <csv_path>");
    }

    /* Open CSV file */
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Cannot open file: %s", argv[1]);
        error_exit(msg);
    }

    /* Read header line */
    if (fgets(line, MAX_LINE_LEN, fp) == NULL) {
        fclose(fp);
        error_exit("Empty CSV file");
    }
    trim(line);
    num_columns = split_csv_line(line, header_fields, MAX_COLUMNS);

    /* Remove any trailing empty columns (from \r\n artifacts) */
    while (num_columns > 0 && strlen(header_fields[num_columns - 1]) == 0) {
        num_columns--;
    }
    if (num_columns == 0) {
        fclose(fp);
        error_exit("No columns found in CSV header");
    }

    /* Find vehicle_id column */
    for (int i = 0; i < num_columns; i++) {
        if (strcmp(header_fields[i], "vehicle_id") == 0) {
            vehicle_id_col = i;
            break;
        }
    }
    if (vehicle_id_col < 0) {
        fclose(fp);
        error_exit("CSV missing required column: vehicle_id");
    }

    /* Build columns array */
    cJSON *columns_arr = cJSON_CreateArray();
    for (int i = 0; i < num_columns; i++) {
        cJSON_AddItemToArray(columns_arr, cJSON_CreateString(header_fields[i]));
    }

    /* Data object: maps vehicle_id -> array of row objects */
    cJSON *data_obj = cJSON_CreateObject();

    /* Vehicle list (ordered, deduplicated) */
    char vehicle_list[64][MAX_FIELD_LEN];
    int num_vehicles = 0;

    /* Read data rows */
    while (fgets(line, MAX_LINE_LEN, fp) != NULL) {
        /* Skip empty lines */
        trim(line);
        if (strlen(line) == 0) {
            continue;
        }

        total_rows++;
        int num_fields = split_csv_line(line, row_fields, MAX_COLUMNS);

        /* Skip rows with wrong number of fields */
        if (num_fields != num_columns) {
            skipped_rows++;
            continue;
        }

        /* Get vehicle_id */
        char *vid = row_fields[vehicle_id_col];
        if (strlen(vid) == 0) {
            skipped_rows++;
            continue;
        }

        /* Track unique vehicles */
        int found = 0;
        for (int i = 0; i < num_vehicles; i++) {
            if (strcmp(vehicle_list[i], vid) == 0) {
                found = 1;
                break;
            }
        }
        if (!found && num_vehicles < 64) {
            strncpy(vehicle_list[num_vehicles], vid, MAX_FIELD_LEN - 1);
            vehicle_list[num_vehicles][MAX_FIELD_LEN - 1] = '\0';
            num_vehicles++;
        }

        /* Get or create vehicle array in data_obj */
        cJSON *vehicle_arr = cJSON_GetObjectItem(data_obj, vid);
        if (vehicle_arr == NULL) {
            vehicle_arr = cJSON_CreateArray();
            cJSON_AddItemToObject(data_obj, vid, vehicle_arr);
        }

        /* Build row object */
        cJSON *row_obj = cJSON_CreateObject();
        for (int i = 0; i < num_columns; i++) {
            if (i == vehicle_id_col) {
                /* vehicle_id is always a string */
                cJSON_AddStringToObject(row_obj, header_fields[i],
                                        row_fields[i]);
            } else if (strcmp(header_fields[i], "timestamp") == 0) {
                /* timestamp is always a string */
                cJSON_AddStringToObject(row_obj, header_fields[i],
                                        row_fields[i]);
            } else if (is_number(row_fields[i])) {
                /* Numeric fields */
                double val = atof(row_fields[i]);
                cJSON_AddNumberToObject(row_obj, header_fields[i], val);
            } else {
                /* Fallback: string */
                cJSON_AddStringToObject(row_obj, header_fields[i],
                                        row_fields[i]);
            }
        }
        cJSON_AddItemToArray(vehicle_arr, row_obj);
        valid_rows++;
    }
    fclose(fp);

    /* Build vehicles array */
    cJSON *vehicles_arr = cJSON_CreateArray();
    for (int i = 0; i < num_vehicles; i++) {
        cJSON_AddItemToArray(vehicles_arr, cJSON_CreateString(vehicle_list[i]));
    }

    /* Build validation object */
    cJSON *validation = cJSON_CreateObject();
    cJSON_AddNumberToObject(validation, "valid_rows", valid_rows);
    cJSON_AddNumberToObject(validation, "skipped_rows", skipped_rows);

    /* Build root output */
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "success");
    cJSON_AddItemToObject(root, "vehicles", vehicles_arr);
    cJSON_AddItemToObject(root, "columns", columns_arr);
    cJSON_AddItemToObject(root, "data", data_obj);
    cJSON_AddNumberToObject(root, "total_rows", total_rows);
    cJSON_AddItemToObject(root, "validation", validation);

    /* Print and cleanup */
    char *output = cJSON_Print(root);
    if (output != NULL) {
        printf("%s\n", output);
        cJSON_free(output);
    }
    cJSON_Delete(root);

    return 0;
}
