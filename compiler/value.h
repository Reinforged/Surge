#ifndef SURGE_VALUE_H
#define SURGE_VALUE_H

typedef enum {
    VALUE_STRING,
    VALUE_INT,
    VALUE_BOOL
} ValueType;

typedef struct {
    ValueType type;

    union {
        char *string;
        long integer;
        int boolean;
    };
} Value;

Value value_string(const char *string);
Value value_int(long integer);
Value value_bool(int boolean);

void value_free(Value *value);
void value_print(const Value *value);

#endif
