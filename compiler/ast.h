#ifndef SURGE_AST_H
#define SURGE_AST_H

#include "lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_STRING,
    AST_INTEGER,
    AST_BOOLEAN,
    AST_CALL,
    AST_VARIABLE_DECLARATION,
    AST_VARIABLE_REFERENCE,
    AST_BINARY,
    AST_UNARY,
    AST_IF,
    AST_WHILE,
    AST_FUNCTION,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE
} AstNodeType;

typedef struct AstNode AstNode;

struct AstNode {
    AstNodeType type;

    union {
        struct {
            AstNode **statements;
            int count;
        } program;

        struct {
            char *value;
        } string;
        
        struct {
            long value;
        } integer;
        
        struct {
                   int value;
               } boolean;

        struct {
            char *name;
            AstNode **arguments;
            int argument_count;
        } call;
        
        struct {
            char *name;
            AstNode *value;
        } variable_declaration;

        struct {
            char *name;
        } variable_reference;
        
        struct {
            AstNode *left;
            TokenType operator;
            AstNode *right;
        } binary;
        
        struct {
            TokenType operator;
            AstNode *operand;
        } unary;
        
        struct {
            AstNode *condition;
            AstNode *body;
            AstNode *else_body;
        } if_statement;
        
        struct {
            AstNode *condition;
            AstNode *body;
        } while_statement;
        
        struct {
            char *name;
            char **parameters;
            int parameter_count;
            AstNode *body;
        } function;
        
        struct {
            AstNode *value;
        } return_statement;
    };
};

AstNode *ast_create_program(void);
void ast_program_add(AstNode *program, AstNode *statement);

AstNode *ast_create_string(const char *value);
AstNode *ast_create_integer(long value);
AstNode *ast_create_boolean(int boolean);

AstNode *ast_create_call(
    const char *name,
    AstNode **arguments,
    int argument_count
);

AstNode *ast_create_variable_declaration(
    const char *name,
    AstNode *value
);

AstNode *ast_create_variable_reference(const char *name);

AstNode *ast_create_binary(
    AstNode *left,
    TokenType operator,
    AstNode *right
);

AstNode *ast_create_unary(
    TokenType operator,
    AstNode *operand
);

AstNode *ast_create_if(
    AstNode *condition,
    AstNode *body,
    AstNode *else_body
);

AstNode *ast_create_while(
    AstNode *condition,
    AstNode *body
);

AstNode *ast_create_function(
    const char *name,
    char **parameters,
    int parameter_count,
    AstNode *body
);

AstNode *ast_create_return(AstNode *value);
AstNode *ast_create_break(void);
AstNode *ast_create_continue(void);
void ast_print(AstNode *node, int indent);
void ast_free(AstNode *node);

#endif
