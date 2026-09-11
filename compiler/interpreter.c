#include "interpreter.h"
#include "value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    Value value;
} Variable;

typedef struct Environment Environment;

struct Environment {
    Variable *variables;
    int count;
    Environment *parent;
};

static Environment *environment_create(Environment *parent)
{
    Environment *environment = malloc(sizeof(Environment));

    if (environment == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    environment->variables = NULL;
    environment->count = 0;
    environment->parent = parent;

    return environment;
}

static Variable *find_variable(Environment *environment, const char *name)
{
    for (Environment *current = environment;
         current != NULL;
         current = current->parent)
    {
        for (int i = 0; i < current->count; i++)
        {
            if (strcmp(current->variables[i].name, name) == 0)
            {
                return &current->variables[i];
            }
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
    Variable *variable = find_variable(environment, name);

    if (variable != NULL)
    {
        value_free(&variable->value);
        variable->value = value;
        return;
    }

    Variable *variables = realloc(
        environment->variables,
        sizeof(Variable) * (environment->count + 1)
    );

    if (variables == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    environment->variables = variables;

    Variable *new_variable =
        &environment->variables[environment->count];

    new_variable->name = malloc(strlen(name) + 1);

    if (new_variable->name == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    strcpy(new_variable->name, name);
    new_variable->value = value;

    environment->count++;
}

static void environment_free(Environment *environment)
{
    for (int i = 0; i < environment->count; i++)
    {
        free(environment->variables[i].name);
        value_free(&environment->variables[i].value);
    }

    free(environment->variables);
    free(environment);
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
        Variable *variable = find_variable(
            environment,
            node->variable_reference.name
        );

        if (variable == NULL)
        {
            fprintf(
                stderr,
                "Surge runtime error: variable '%s' is not defined.\n",
                node->variable_reference.name
            );
            exit(1);
        }

        if (variable->value.type == VALUE_STRING)
        {
            return value_string(variable->value.string);
        }

        return variable->value;
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
                Environment *block =
                    environment_create(environment);

                execute(node->if_statement.body, block);
                environment_free(block);
            }
            else if (node->if_statement.else_body != NULL)
            {
                Environment *block =
                    environment_create(environment);

                execute(node->if_statement.else_body, block);
                environment_free(block);
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

                Environment *block =
                    environment_create(environment);

                execute(node->while_statement.body, block);

                environment_free(block);
            }

            break;
        }
    }
}

void interpreter_run(AstNode *program)
{
    Environment *environment =
        environment_create(NULL);

    execute(program, environment);

    environment_free(environment);
}
