#include "value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *string)
{
    size_t length = strlen(string);

    char *copy = malloc(length + 1);

    if (copy == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    memcpy(copy, string, length + 1);

    return copy;
}

Value value_string(const char *string)
{
    Value value;

    value.type = VALUE_STRING;
    value.string = copy_string(string);

    return value;
}

Value value_int(long integer)
{
    Value value;

    value.type = VALUE_INT;
    value.integer = integer;

    return value;
}

Value value_bool(int boolean)
{
    Value value;

    value.type = VALUE_BOOL;
    value.boolean = boolean != 0;

    return value;
}

void value_free(Value *value)
{
    if (value->type == VALUE_STRING)
    {
        free(value->string);
        value->string = NULL;
    }
}

void value_print(const Value *value)
{
    switch (value->type)
    {
        case VALUE_STRING:
            printf("%s\n", value->string);
            break;

        case VALUE_INT:
            printf("%ld\n", value->integer);
            break;

        case VALUE_BOOL:
            printf("%s\n", value->boolean ? "true" : "false");
            break;
    }
}
