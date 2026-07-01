/*
 * json_utils.h — Vendored cJSON Header
 *
 * Lightweight JSON parser/generator for C.
 * Based on cJSON by Dave Gamble (MIT License).
 * Provides all JSON operations needed by Phase 4 C agents.
 */

#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* cJSON type constants */
#define cJSON_Invalid   (0)
#define cJSON_False     (1 << 0)
#define cJSON_True      (1 << 1)
#define cJSON_NULL      (1 << 2)
#define cJSON_Number    (1 << 3)
#define cJSON_String    (1 << 4)
#define cJSON_Array     (1 << 5)
#define cJSON_Object    (1 << 6)
#define cJSON_Raw       (1 << 7)

/* The cJSON structure */
typedef struct cJSON {
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;        /* child pointer for array/object */
    int type;                   /* type of this item */
    char *valuestring;          /* value if type == cJSON_String or cJSON_Raw */
    double valuedouble;         /* value if type == cJSON_Number */
    char *string;               /* key name if this is a child of an object */
} cJSON;

/* ---- Parsing ---- */
cJSON *cJSON_Parse(const char *value);

/* ---- Printing ---- */
char *cJSON_Print(const cJSON *item);
char *cJSON_PrintUnformatted(const cJSON *item);

/* ---- Deletion ---- */
void cJSON_Delete(cJSON *item);
void cJSON_free(void *ptr);

/* ---- Creation ---- */
cJSON *cJSON_CreateNull(void);
cJSON *cJSON_CreateTrue(void);
cJSON *cJSON_CreateFalse(void);
cJSON *cJSON_CreateBool(int boolean);
cJSON *cJSON_CreateNumber(double num);
cJSON *cJSON_CreateString(const char *string);
cJSON *cJSON_CreateArray(void);
cJSON *cJSON_CreateObject(void);

/* ---- Array operations ---- */
void cJSON_AddItemToArray(cJSON *array, cJSON *item);
cJSON *cJSON_GetArrayItem(const cJSON *array, int index);
int cJSON_GetArraySize(const cJSON *array);

/* ---- Object operations ---- */
void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);

/* ---- Convenience add-to-object macros ---- */
#define cJSON_AddNullToObject(object, name) \
    cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object, name) \
    cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object, name) \
    cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object, name, b) \
    cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object, name, n) \
    cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object, name, s) \
    cJSON_AddItemToObject(object, name, cJSON_CreateString(s))

/* ---- Type checks ---- */
int cJSON_IsInvalid(const cJSON *item);
int cJSON_IsFalse(const cJSON *item);
int cJSON_IsTrue(const cJSON *item);
int cJSON_IsBool(const cJSON *item);
int cJSON_IsNull(const cJSON *item);
int cJSON_IsNumber(const cJSON *item);
int cJSON_IsString(const cJSON *item);
int cJSON_IsArray(const cJSON *item);
int cJSON_IsObject(const cJSON *item);

/* ---- Iteration helper ---- */
/* Usage: cJSON *item; cJSON_ArrayForEach(item, array) { ... } */
#define cJSON_ArrayForEach(element, array) \
    for (element = (array != NULL) ? (array)->child : NULL; \
         element != NULL; \
         element = element->next)

#ifdef __cplusplus
}
#endif

#endif /* JSON_UTILS_H */
