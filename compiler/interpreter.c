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
    Value *existing = find_variable(
        environment,
        name
    );

    if (existing != NULL)
    {
        value_free(existing);
        *existing = value;
        return;
    }
    
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
    
    if (node->type == AST_BOOLEAN)
    {
        return value_bool(node->boolean.value);
    }
    
    if (node->type == AST_UNARY)
    {
        Value operand = evaluate(
            node->unary.operand,
            environment
        );

        if (operand.type != VALUE_BOOL)
        {
            fprintf(
                stderr,
                "Surge runtime error: 'not' requires a boolean value.\n"
            );
            exit(1);
        }

        switch (node->unary.operator)
        {
            case TOKEN_NOT:
                return value_bool(!operand.boolean);

            default:
                fprintf(
                    stderr,
                    "Surge runtime error: unknown unary operator.\n"
                );
                exit(1);
        }
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
                "Surge runtime error: variable '%s' is not defined.\n",
                node->variable_reference.name
            );
            exit(1);
        }

        return *value;
    }

    if (node->type == AST_BINARY)
    {
        if (node->binary.operator == TOKEN_AND)
        {
            Value left = evaluate(
                node->binary.left,
                environment
            );

            if (left.type != VALUE_BOOL)
            {
                fprintf(
                    stderr,
                    "Surge runtime error: 'and' requires boolean values.\n"
                );
                exit(1);
            }

            if (!left.boolean)
            {
                return value_bool(0);
            }

            Value right = evaluate(
                node->binary.right,
                environment
            );

            if (right.type != VALUE_BOOL)
            {
                fprintf(
                    stderr,
                    "Surge runtime error: 'and' requires boolean values.\n"
                );
                exit(1);
            }

            return value_bool(right.boolean);
        }

        if (node->binary.operator == TOKEN_OR)
        {
            Value left = evaluate(
                node->binary.left,
                environment
            );

            if (left.type != VALUE_BOOL)
            {
                fprintf(
                    stderr,
                    "Surge runtime error: 'or' requires boolean values.\n"
                );
                exit(1);
            }

            if (left.boolean)
            {
                return value_bool(1);
            }

            Value right = evaluate(
                node->binary.right,
                environment
            );

            if (right.type != VALUE_BOOL)
            {
                fprintf(
                    stderr,
                    "Surge runtime error: 'or' requires boolean values.\n"
                );
                exit(1);
            }

            return value_bool(right.boolean);
        }

        Value left = evaluate(
            node->binary.left,
            environment
        );

        Value right = evaluate(
            node->binary.right,
            environment
        );

        if (left.type != VALUE_INT || right.type != VALUE_INT)
        {
            fprintf(
                stderr,
                "Surge runtime error: comparison and arithmetic require integer values.\n"
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

            case TOKEN_EQUAL_EQUAL:
                return value_bool(left.integer == right.integer);

            case TOKEN_BANG_EQUAL:
                return value_bool(left.integer != right.integer);

            case TOKEN_LESS:
                return value_bool(left.integer < right.integer);

            case TOKEN_LESS_EQUAL:
                return value_bool(left.integer <= right.integer);

            case TOKEN_GREATER:
                return value_bool(left.integer > right.integer);

            case TOKEN_GREATER_EQUAL:
                return value_bool(left.integer >= right.integer);

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
        case AST_BOOLEAN:
        case AST_VARIABLE_REFERENCE:
        case AST_BINARY:
        case AST_UNARY:
            break;

        case AST_IF:
        {
            Value condition = evaluate(
                node->if_statement.condition,
                environment
            );

            if (condition.type != VALUE_BOOL)
            {
                fprintf(
                    stderr,
                    "Surge runtime error: if condition must be a boolean.\n"
                );
                exit(1);
            }

            if (condition.boolean)
            {
                execute(
                    node->if_statement.body,
                    environment
                );
            }
            else if (node->if_statement.else_body != NULL)
            {
                execute(
                    node->if_statement.else_body,
                    environment
                );
            }

            break;
        }
            
        case AST_WHILE:
        {
            while (1)
            {
                Value condition = evaluate(
                    node->while_statement.condition,
                    environment
                );

                if (condition.type != VALUE_BOOL)
                {
                    fprintf(
                        stderr,
                        "Surge runtime error: while condition must be a boolean.\n"
                    );
                    exit(1);
                }

                if (!condition.boolean)
                {
                    break;
                }

                execute(
                    node->while_statement.body,
                    environment
                );
            }

            break;
        }
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
