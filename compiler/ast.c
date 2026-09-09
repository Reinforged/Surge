#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *source)
{
    size_t length = strlen(source);

    char *copy = malloc(length + 1);

    if (copy == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    memcpy(copy, source, length + 1);

    return copy;
}

AstNode *ast_create_string(const char *value)
{
    AstNode *node = malloc(sizeof(AstNode));

    if (node == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    node->type = AST_STRING;
    node->string.value = copy_string(value);

    return node;
}

AstNode *ast_create_call(const char *name, AstNode *argument)
{
    AstNode *node = malloc(sizeof(AstNode));

    if (node == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    node->type = AST_CALL;
    node->call.name = copy_string(name);
    node->call.argument = argument;

    return node;
}

static void print_indent(int indent)
{
    for (int i = 0; i < indent; i++)
    {
        printf("  ");
    }
}

void ast_print(AstNode *node, int indent)
{
    if (node == NULL)
    {
        return;
    }

    print_indent(indent);

    switch (node->type)
    {
        case AST_STRING:
            printf("StringLiteral: \"%s\"\n", node->string.value);
            break;

        case AST_CALL:
            printf("CallExpression: %s\n", node->call.name);

            print_indent(indent + 1);
            printf("argument:\n");

            ast_print(node->call.argument, indent + 2);
            break;
    }
}

void ast_free(AstNode *node)
{
    if (node == NULL)
    {
        return;
    }

    switch (node->type)
    {
        case AST_STRING:
            free(node->string.value);
            break;

        case AST_CALL:
            free(node->call.name);
            ast_free(node->call.argument);
            break;
    }

    free(node);
}
