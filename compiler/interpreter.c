#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    AstNode *value;
} Variable;

typedef struct {
    Variable *variables;
    int count;
} Environment;

static AstNode *find_variable(
    Environment *environment,
    const char *name
)
{
    for (int i = 0; i < environment->count; i++)
    {
        if (strcmp(environment->variables[i].name, name) == 0)
        {
            return environment->variables[i].value;
        }
    }

    return NULL;
}

static void set_variable(
    Environment *environment,
    const char *name,
    AstNode *value
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

static AstNode *evaluate(
    AstNode *node,
    Environment *environment
)
{
    if (node->type == AST_STRING)
    {
        return node;
    }

    if (node->type == AST_VARIABLE_REFERENCE)
    {
        AstNode *value = find_variable(
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

        return value;
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
            AstNode *value = evaluate(
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
                AstNode *argument = evaluate(
                    node->call.argument,
                    environment
                );

                if (argument->type == AST_STRING)
                {
                    printf(
                        "%s\n",
                        argument->string.value
                    );
                }
                else
                {
                    fprintf(
                        stderr,
                        "Surge runtime error: "
                        "print() received an unsupported value.\n"
                    );

                    exit(1);
                }
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
        case AST_VARIABLE_REFERENCE:
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
