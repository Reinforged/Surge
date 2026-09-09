#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *source) {
    size_t length = strlen(source);

    char *result = malloc(length + 1);

    if (result == NULL) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    memcpy(result, source, length + 1);

    return result;
}

AstNode *ast_create_program(void) {
    AstNode *node = malloc(sizeof(AstNode));

    if (node == NULL) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    node->type = AST_PROGRAM;
    node->program.statements = NULL;
    node->program.count = 0;

    return node;
}

void ast_program_add(AstNode *program, AstNode *statement) {
    int new_count = program->program.count + 1;

    AstNode **new_statements = realloc(
        program->program.statements,
        sizeof(AstNode *) * (size_t)new_count
    );

    if (new_statements == NULL) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    program->program.statements = new_statements;
    program->program.statements[new_count - 1] = statement;
    program->program.count = new_count;
}

AstNode *ast_create_string(const char *value) {
    AstNode *node = malloc(sizeof(AstNode));

    if (node == NULL) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    node->type = AST_STRING;
    node->string.value = copy_string(value);

    return node;
}

AstNode *ast_create_integer(long value)
{
    AstNode *node = malloc(sizeof(AstNode));

    if (node == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    node->type = AST_INTEGER;
    node->integer.value = value;

    return node;
}

AstNode *ast_create_call(const char *name, AstNode *argument) {
    AstNode *node = malloc(sizeof(AstNode));

    if (node == NULL) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    node->type = AST_CALL;
    node->call.name = copy_string(name);
    node->call.argument = argument;

    return node;
}

AstNode *ast_create_variable_declaration(
    const char *name,
    AstNode *value
) {
    AstNode *node = malloc(sizeof(AstNode));

    if (node == NULL) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    node->type = AST_VARIABLE_DECLARATION;
    node->variable_declaration.name = copy_string(name);
    node->variable_declaration.value = value;

    return node;
}

AstNode *ast_create_variable_reference(const char *name) {
    AstNode *node = malloc(sizeof(AstNode));

    if (node == NULL) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    node->type = AST_VARIABLE_REFERENCE;
    node->variable_reference.name = copy_string(name);

    return node;
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void ast_print(AstNode *node, int indent) {
    if (node == NULL) {
        return;
    }

    print_indent(indent);

    switch (node->type) {
        case AST_PROGRAM:
            printf("Program\n");

            for (int i = 0; i < node->program.count; i++) {
                ast_print(node->program.statements[i], indent + 1);
            }
            break;

        case AST_STRING:
            printf("StringLiteral: \"%s\"\n", node->string.value);
            break;
            
        case AST_INTEGER:
            printf("IntegerLiteral: %ld\n", node->integer.value);
            break;

        case AST_CALL:
            printf("CallExpression: %s\n", node->call.name);

            print_indent(indent + 1);
            printf("argument:\n");

            ast_print(node->call.argument, indent + 2);
            break;
            
        case AST_VARIABLE_DECLARATION:
            printf("VariableDeclaration: %s\n",
                   node->variable_declaration.name);

            print_indent(indent + 1);
            printf("value:\n");

            ast_print(node->variable_declaration.value, indent + 2);
            break;

        case AST_VARIABLE_REFERENCE:
            printf("VariableReference: %s\n",
                   node->variable_reference.name);
            break;
    }
}

void ast_free(AstNode *node) {
    if (node == NULL) {
        return;
    }

    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.count; i++) {
                ast_free(node->program.statements[i]);
            }

            free(node->program.statements);
            break;

        case AST_STRING:
            free(node->string.value);
            break;
            
        case AST_INTEGER:
            break;

        case AST_CALL:
            free(node->call.name);
            ast_free(node->call.argument);
            break;
            
        case AST_VARIABLE_DECLARATION:
            free(node->variable_declaration.name);
            ast_free(node->variable_declaration.value);
            break;

        case AST_VARIABLE_REFERENCE:
            free(node->variable_reference.name);
            break;
    }

    free(node);
}
