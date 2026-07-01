/*
 * json_utils.c — Vendored cJSON Implementation
 *
 * Lightweight JSON parser/generator for C.
 * Based on cJSON by Dave Gamble (MIT License).
 *
 * This is a purpose-built implementation covering all JSON operations
 * needed by Phase 4 C agents: parsing, creation, manipulation, printing.
 */

#include "json_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

/* ========================================================================
 * Internal helpers
 * ======================================================================== */

static cJSON *cJSON_New_Item(void) {
    cJSON *node = (cJSON *)calloc(1, sizeof(cJSON));
    return node;
}

static char *cJSON_strdup(const char *str) {
    size_t len;
    char *copy;
    if (str == NULL) {
        return NULL;
    }
    len = strlen(str) + 1;
    copy = (char *)malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, str, len);
    return copy;
}

/* ========================================================================
 * Deletion
 * ======================================================================== */

void cJSON_Delete(cJSON *item) {
    cJSON *next;
    while (item != NULL) {
        next = item->next;
        if (item->child != NULL) {
            cJSON_Delete(item->child);
        }
        if (item->valuestring != NULL) {
            free(item->valuestring);
        }
        if (item->string != NULL) {
            free(item->string);
        }
        free(item);
        item = next;
    }
}

void cJSON_free(void *ptr) {
    free(ptr);
}

/* ========================================================================
 * Parsing — internal functions
 * ======================================================================== */

/* Skip whitespace */
static const char *skip_whitespace(const char *in) {
    if (in == NULL) {
        return NULL;
    }
    while (*in != '\0' && (unsigned char)*in <= ' ') {
        in++;
    }
    return in;
}

/* Forward declarations */
static const char *parse_value(cJSON *item, const char *value);
static const char *parse_string(cJSON *item, const char *str);
static const char *parse_number(cJSON *item, const char *num);
static const char *parse_array(cJSON *item, const char *value);
static const char *parse_object(cJSON *item, const char *value);

/* Parse a JSON string value */
static const char *parse_string(cJSON *item, const char *str) {
    const char *ptr = str + 1; /* skip opening quote */
    char *out;
    size_t len = 0;
    const char *ptr2;

    if (*str != '\"') {
        return NULL;
    }

    /* Calculate length */
    ptr2 = ptr;
    while (*ptr2 != '\"' && *ptr2 != '\0') {
        if (*ptr2 == '\\') {
            ptr2++;
        }
        ptr2++;
        len++;
    }

    out = (char *)malloc(len + 1);
    if (out == NULL) {
        return NULL;
    }

    ptr2 = ptr;
    char *out2 = out;
    while (*ptr2 != '\"' && *ptr2 != '\0') {
        if (*ptr2 != '\\') {
            *out2++ = *ptr2++;
        } else {
            ptr2++;
            switch (*ptr2) {
                case 'b':  *out2++ = '\b'; break;
                case 'f':  *out2++ = '\f'; break;
                case 'n':  *out2++ = '\n'; break;
                case 'r':  *out2++ = '\r'; break;
                case 't':  *out2++ = '\t'; break;
                case '\"': *out2++ = '\"'; break;
                case '\\': *out2++ = '\\'; break;
                case '/':  *out2++ = '/';  break;
                default:   *out2++ = *ptr2; break;
            }
            ptr2++;
        }
    }
    *out2 = '\0';

    if (*ptr2 == '\"') {
        ptr2++;
    }

    item->type = cJSON_String;
    item->valuestring = out;
    return ptr2;
}

/* Parse a JSON number */
static const char *parse_number(cJSON *item, const char *num) {
    double result = 0.0;
    double sign = 1.0;
    double scale = 0.0;
    int subscale = 0;
    int signsubscale = 1;

    if (*num == '-') {
        sign = -1.0;
        num++;
    }
    if (*num == '0') {
        num++;
    }
    if (*num >= '1' && *num <= '9') {
        do {
            result = (result * 10.0) + (*num - '0');
            num++;
        } while (*num >= '0' && *num <= '9');
    }
    if (*num == '.') {
        num++;
        while (*num >= '0' && *num <= '9') {
            result = (result * 10.0) + (*num - '0');
            scale--;
            num++;
        }
    }
    if (*num == 'e' || *num == 'E') {
        num++;
        if (*num == '+') {
            num++;
        } else if (*num == '-') {
            signsubscale = -1;
            num++;
        }
        while (*num >= '0' && *num <= '9') {
            subscale = (subscale * 10) + (*num - '0');
            num++;
        }
    }

    result = sign * result * pow(10.0, scale + subscale * signsubscale);

    item->type = cJSON_Number;
    item->valuedouble = result;
    return num;
}

/* Parse a JSON array */
static const char *parse_array(cJSON *item, const char *value) {
    cJSON *child;

    item->type = cJSON_Array;
    value = skip_whitespace(value + 1); /* skip '[' */
    if (*value == ']') {
        return value + 1; /* empty array */
    }

    item->child = child = cJSON_New_Item();
    if (child == NULL) {
        return NULL;
    }
    value = skip_whitespace(parse_value(child, skip_whitespace(value)));
    if (value == NULL) {
        return NULL;
    }

    while (*value == ',') {
        cJSON *new_item = cJSON_New_Item();
        if (new_item == NULL) {
            return NULL;
        }
        child->next = new_item;
        new_item->prev = child;
        child = new_item;
        value = skip_whitespace(parse_value(child, skip_whitespace(value + 1)));
        if (value == NULL) {
            return NULL;
        }
    }

    if (*value == ']') {
        return value + 1;
    }
    return NULL; /* malformed */
}

/* Parse a JSON object */
static const char *parse_object(cJSON *item, const char *value) {
    cJSON *child;

    item->type = cJSON_Object;
    value = skip_whitespace(value + 1); /* skip '{' */
    if (*value == '}') {
        return value + 1; /* empty object */
    }

    item->child = child = cJSON_New_Item();
    if (child == NULL) {
        return NULL;
    }

    /* Parse key */
    value = skip_whitespace(parse_string(child, skip_whitespace(value)));
    if (value == NULL) {
        return NULL;
    }
    child->string = child->valuestring;
    child->valuestring = NULL;

    if (*value != ':') {
        return NULL;
    }
    value = skip_whitespace(parse_value(child, skip_whitespace(value + 1)));
    if (value == NULL) {
        return NULL;
    }

    while (*value == ',') {
        cJSON *new_item = cJSON_New_Item();
        if (new_item == NULL) {
            return NULL;
        }
        child->next = new_item;
        new_item->prev = child;
        child = new_item;

        value = skip_whitespace(parse_string(child, skip_whitespace(value + 1)));
        if (value == NULL) {
            return NULL;
        }
        child->string = child->valuestring;
        child->valuestring = NULL;

        if (*value != ':') {
            return NULL;
        }
        value = skip_whitespace(parse_value(child, skip_whitespace(value + 1)));
        if (value == NULL) {
            return NULL;
        }
    }

    if (*value == '}') {
        return value + 1;
    }
    return NULL; /* malformed */
}

/* Parse any JSON value */
static const char *parse_value(cJSON *item, const char *value) {
    if (value == NULL) {
        return NULL;
    }

    if (strncmp(value, "null", 4) == 0) {
        item->type = cJSON_NULL;
        return value + 4;
    }
    if (strncmp(value, "false", 5) == 0) {
        item->type = cJSON_False;
        item->valuedouble = 0.0;
        return value + 5;
    }
    if (strncmp(value, "true", 4) == 0) {
        item->type = cJSON_True;
        item->valuedouble = 1.0;
        return value + 4;
    }
    if (*value == '\"') {
        return parse_string(item, value);
    }
    if (*value == '-' || (*value >= '0' && *value <= '9')) {
        return parse_number(item, value);
    }
    if (*value == '[') {
        return parse_array(item, value);
    }
    if (*value == '{') {
        return parse_object(item, value);
    }

    return NULL; /* failure */
}

/* ========================================================================
 * Public parsing API
 * ======================================================================== */

cJSON *cJSON_Parse(const char *value) {
    cJSON *item;
    const char *end;

    if (value == NULL) {
        return NULL;
    }

    item = cJSON_New_Item();
    if (item == NULL) {
        return NULL;
    }

    end = parse_value(item, skip_whitespace(value));
    if (end == NULL) {
        cJSON_Delete(item);
        return NULL;
    }

    return item;
}

/* ========================================================================
 * Printing — internal functions
 * ======================================================================== */

/* Dynamic string buffer for printing */
typedef struct {
    char *buffer;
    size_t length;
    size_t capacity;
} printbuffer;

static int ensure_capacity(printbuffer *buf, size_t needed) {
    if (buf->length + needed + 1 > buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        if (new_cap < buf->length + needed + 1) {
            new_cap = buf->length + needed + 64;
        }
        char *new_buf = (char *)realloc(buf->buffer, new_cap);
        if (new_buf == NULL) {
            return 0;
        }
        buf->buffer = new_buf;
        buf->capacity = new_cap;
    }
    return 1;
}

static void buf_append(printbuffer *buf, const char *str) {
    size_t len = strlen(str);
    if (ensure_capacity(buf, len)) {
        memcpy(buf->buffer + buf->length, str, len);
        buf->length += len;
        buf->buffer[buf->length] = '\0';
    }
}

static void buf_append_char(printbuffer *buf, char c) {
    if (ensure_capacity(buf, 1)) {
        buf->buffer[buf->length] = c;
        buf->length++;
        buf->buffer[buf->length] = '\0';
    }
}

static void print_value(const cJSON *item, printbuffer *buf, int depth, int fmt);

static void print_string(const char *str, printbuffer *buf) {
    if (str == NULL) {
        buf_append(buf, "\"\"");
        return;
    }
    buf_append_char(buf, '\"');
    const char *ptr = str;
    while (*ptr != '\0') {
        if ((unsigned char)*ptr > 31 && *ptr != '\"' && *ptr != '\\') {
            buf_append_char(buf, *ptr);
        } else {
            buf_append_char(buf, '\\');
            switch (*ptr) {
                case '\\': buf_append_char(buf, '\\'); break;
                case '\"': buf_append_char(buf, '\"'); break;
                case '\b': buf_append_char(buf, 'b');  break;
                case '\f': buf_append_char(buf, 'f');  break;
                case '\n': buf_append_char(buf, 'n');  break;
                case '\r': buf_append_char(buf, 'r');  break;
                case '\t': buf_append_char(buf, 't');  break;
                default:   buf_append_char(buf, *ptr); break;
            }
        }
        ptr++;
    }
    buf_append_char(buf, '\"');
}

static void print_number(const cJSON *item, printbuffer *buf) {
    char num_buf[64];
    double d = item->valuedouble;

    if (d == 0.0) {
        /* Handle negative zero */
        if (signbit(d)) {
            snprintf(num_buf, sizeof(num_buf), "-0");
        } else {
            snprintf(num_buf, sizeof(num_buf), "0");
        }
    } else if (fabs(d) < 1.0e-6 || fabs(d) > 1.0e9) {
        snprintf(num_buf, sizeof(num_buf), "%e", d);
    } else if (floor(d) == d && fabs(d) < 1.0e15) {
        snprintf(num_buf, sizeof(num_buf), "%.0f", d);
    } else {
        snprintf(num_buf, sizeof(num_buf), "%g", d);
    }
    buf_append(buf, num_buf);
}

static void print_indent(printbuffer *buf, int depth) {
    int i;
    for (i = 0; i < depth; i++) {
        buf_append_char(buf, '\t');
    }
}

static void print_array(const cJSON *item, printbuffer *buf, int depth, int fmt) {
    cJSON *child = item->child;

    buf_append_char(buf, '[');
    if (fmt) {
        buf_append_char(buf, '\n');
    }

    while (child != NULL) {
        if (fmt) {
            print_indent(buf, depth + 1);
        }
        print_value(child, buf, depth + 1, fmt);
        if (child->next != NULL) {
            buf_append_char(buf, ',');
        }
        if (fmt) {
            buf_append_char(buf, '\n');
        }
        child = child->next;
    }

    if (fmt) {
        print_indent(buf, depth);
    }
    buf_append_char(buf, ']');
}

static void print_object(const cJSON *item, printbuffer *buf, int depth, int fmt) {
    cJSON *child = item->child;

    buf_append_char(buf, '{');
    if (fmt) {
        buf_append_char(buf, '\n');
    }

    while (child != NULL) {
        if (fmt) {
            print_indent(buf, depth + 1);
        }
        print_string(child->string, buf);
        buf_append_char(buf, ':');
        if (fmt) {
            buf_append_char(buf, '\t');
        }
        print_value(child, buf, depth + 1, fmt);
        if (child->next != NULL) {
            buf_append_char(buf, ',');
        }
        if (fmt) {
            buf_append_char(buf, '\n');
        }
        child = child->next;
    }

    if (fmt) {
        print_indent(buf, depth);
    }
    buf_append_char(buf, '}');
}

static void print_value(const cJSON *item, printbuffer *buf, int depth, int fmt) {
    if (item == NULL) {
        buf_append(buf, "null");
        return;
    }
    switch (item->type) {
        case cJSON_NULL:   buf_append(buf, "null");  break;
        case cJSON_False:  buf_append(buf, "false"); break;
        case cJSON_True:   buf_append(buf, "true");  break;
        case cJSON_Number: print_number(item, buf);  break;
        case cJSON_String: print_string(item->valuestring, buf); break;
        case cJSON_Array:  print_array(item, buf, depth, fmt);   break;
        case cJSON_Object: print_object(item, buf, depth, fmt);  break;
        default: break;
    }
}

/* ========================================================================
 * Public printing API
 * ======================================================================== */

static char *print_internal(const cJSON *item, int fmt) {
    printbuffer buf;
    buf.capacity = 256;
    buf.length = 0;
    buf.buffer = (char *)malloc(buf.capacity);
    if (buf.buffer == NULL) {
        return NULL;
    }
    buf.buffer[0] = '\0';

    print_value(item, &buf, 0, fmt);
    return buf.buffer;
}

char *cJSON_Print(const cJSON *item) {
    return print_internal(item, 1);
}

char *cJSON_PrintUnformatted(const cJSON *item) {
    return print_internal(item, 0);
}

/* ========================================================================
 * Creation functions
 * ======================================================================== */

cJSON *cJSON_CreateNull(void) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_NULL;
    }
    return item;
}

cJSON *cJSON_CreateTrue(void) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_True;
        item->valuedouble = 1.0;
    }
    return item;
}

cJSON *cJSON_CreateFalse(void) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_False;
        item->valuedouble = 0.0;
    }
    return item;
}

cJSON *cJSON_CreateBool(int boolean) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = boolean ? cJSON_True : cJSON_False;
        item->valuedouble = boolean ? 1.0 : 0.0;
    }
    return item;
}

cJSON *cJSON_CreateNumber(double num) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_Number;
        item->valuedouble = num;
    }
    return item;
}

cJSON *cJSON_CreateString(const char *string) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_String;
        item->valuestring = cJSON_strdup(string != NULL ? string : "");
    }
    return item;
}

cJSON *cJSON_CreateArray(void) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_Array;
    }
    return item;
}

cJSON *cJSON_CreateObject(void) {
    cJSON *item = cJSON_New_Item();
    if (item) {
        item->type = cJSON_Object;
    }
    return item;
}

/* ========================================================================
 * Array operations
 * ======================================================================== */

static void suffix_item(cJSON *prev, cJSON *item) {
    prev->next = item;
    item->prev = prev;
}

void cJSON_AddItemToArray(cJSON *array, cJSON *item) {
    cJSON *child;
    if (array == NULL || item == NULL) {
        return;
    }
    child = array->child;
    if (child == NULL) {
        array->child = item;
        item->prev = item;  /* circular for last-element tracking */
        item->next = NULL;
    } else {
        /* Find last element */
        if (child->prev != NULL) {
            suffix_item(child->prev, item);
            child->prev = item;  /* update circular last pointer */
        }
    }
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int index) {
    cJSON *current;
    if (array == NULL) {
        return NULL;
    }
    current = array->child;
    while (current != NULL && index > 0) {
        index--;
        current = current->next;
    }
    return current;
}

int cJSON_GetArraySize(const cJSON *array) {
    cJSON *child;
    int size = 0;
    if (array == NULL) {
        return 0;
    }
    child = array->child;
    while (child != NULL) {
        size++;
        child = child->next;
    }
    return size;
}

/* ========================================================================
 * Object operations
 * ======================================================================== */

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) {
    if (object == NULL || item == NULL || string == NULL) {
        return;
    }
    if (item->string != NULL) {
        free(item->string);
    }
    item->string = cJSON_strdup(string);
    cJSON_AddItemToArray(object, item); /* objects use same linked list */
}

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string) {
    cJSON *current;
    if (object == NULL || string == NULL) {
        return NULL;
    }
    current = object->child;
    while (current != NULL) {
        if (current->string != NULL && strcmp(current->string, string) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* ========================================================================
 * Type checks
 * ======================================================================== */

int cJSON_IsInvalid(const cJSON *item) {
    return (item == NULL) || (item->type == cJSON_Invalid);
}

int cJSON_IsFalse(const cJSON *item) {
    return (item != NULL) && (item->type == cJSON_False);
}

int cJSON_IsTrue(const cJSON *item) {
    return (item != NULL) && (item->type == cJSON_True);
}

int cJSON_IsBool(const cJSON *item) {
    return (item != NULL) && ((item->type == cJSON_True) || (item->type == cJSON_False));
}

int cJSON_IsNull(const cJSON *item) {
    return (item != NULL) && (item->type == cJSON_NULL);
}

int cJSON_IsNumber(const cJSON *item) {
    return (item != NULL) && (item->type == cJSON_Number);
}

int cJSON_IsString(const cJSON *item) {
    return (item != NULL) && (item->type == cJSON_String);
}

int cJSON_IsArray(const cJSON *item) {
    return (item != NULL) && (item->type == cJSON_Array);
}

int cJSON_IsObject(const cJSON *item) {
    return (item != NULL) && (item->type == cJSON_Object);
}
