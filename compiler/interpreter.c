#include "interpreter.h"
#include "value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    Value value;
} Variable;

typedef struct {
    Variable *variables;
    int count;
} Environment;

static Value *find_variable(
    Environment *environment,
    const char *name
)
{
    for (int i = 0; i < environment->count; i++)
    {
        if (strcmp(environment->variables[i].name, name) == 0)
        {
            return &environment->variables[i].value;
        }
    }

    return NULL;
}

static void set_variable(
    Environment *environment,
    const char *name,
    Value value
)
{
    Variable *new_variables = realloc(
        environment->variables,
        sizeof(Variable) * (size_t)(environment->count + 1)
    );

    if (new_variables == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    environment->variables = new_variables;

    environment->variables[environment->count].name =
        malloc(strlen(name) + 1);

    if (environment->variables[environment->count].name == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    strcpy(
        environment->variables[environment->count].name,
        name
    );

    environment->variables[environment->count].value = value;
    environment->count++;
}

static Value evaluate(
    AstNode *node,
    Environment *environment
)
{
    if (node->type == AST_STRING)
    {
        return value_string(node->string.value);
    }
    
    if (node->type == AST_INTEGER)
    {
        return value_int(node->integer.value);
    }

    if (node->type == AST_VARIABLE_REFERENCE)
    {
        Value *value = find_variable(
            environment,
            node->variable_reference.name
        );

        if (value == NULL)
        {
            fprintf(
                stderr,
                "Surge runtime error: variable '%s' "
                "is not defined.\n",
                node->variable_reference.name
            );

            exit(1);
        }

        return *value;
    }
 
    if (node->type == AST_BINARY)
    {
        Value left = evaluate(node->binary.left, environment);
        Value right = evaluate(node->binary.right, environment);

        if (left.type != VALUE_INT || right.type != VALUE_INT)
        {
            fprintf(
                stderr,
                "Surge runtime error: arithmetic requires integer values.\n"
            );
            exit(1);
        }

        switch (node->binary.operator)
        {
            case TOKEN_PLUS:
                return value_int(left.integer + right.integer);

            case TOKEN_MINUS:
                return value_int(left.integer - right.integer);

            case TOKEN_STAR:
                return value_int(left.integer * right.integer);

            case TOKEN_SLASH:
                if (right.integer == 0)
                {
                    fprintf(
                        stderr,
                        "Surge runtime error: division by zero.\n"
                    );
                    exit(1);
                }

                return value_int(left.integer / right.integer);

            default:
                fprintf(
                    stderr,
                    "Surge runtime error: unknown binary operator.\n"
                );
                exit(1);
        }
    }

    fprintf(
        stderr,
        "Surge runtime error: invalid expression.\n"
    );

    exit(1);
}

static void execute(
    AstNode *node,
    Environment *environment
)
{
    if (node == NULL)
    {
        return;
    }

    switch (node->type)
    {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.count; i++)
            {
                execute(
                    node->program.statements[i],
                    environment
                );
            }
            break;

        case AST_VARIABLE_DECLARATION:
        {
            Value value = evaluate(
                node->variable_declaration.value,
                environment
            );

            set_variable(
                environment,
                node->variable_declaration.name,
                value
            );

            break;
        }

        case AST_CALL:
            if (strcmp(node->call.name, "print") == 0)
            {
                Value argument = evaluate(
                    node->call.argument,
                    environment
                );

                value_print(&argument);
            }
            else
            {
                fprintf(
                    stderr,
                    "Surge runtime error: "
                    "unknown function '%s'.\n",
                    node->call.name
                );

                exit(1);
            }

            break;

        case AST_STRING:
        case AST_INTEGER:
        case AST_VARIABLE_REFERENCE:
        case AST_BINARY:
            break;
    }
}

void interpreter_run(AstNode *program)
{
    Environment environment;

    environment.variables = NULL;
    environment.count = 0;

    execute(program, &environment);

    for (int i = 0; i < environment.count; i++)
    {
        free(environment.variables[i].name);
    }

    free(environment.variables);
}
