#ifndef SURGE_AST_H
#define SURGE_AST_H

typedef enum {
    AST_PROGRAM,
    AST_STRING,
    AST_CALL,
    AST_VARIABLE_DECLARATION,
    AST_VARIABLE_REFERENCE
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
            char *name;
            AstNode *argument;
        } call;
        
        struct {
            char *name;
            AstNode *value;
        } variable_declaration;

        struct {
            char *name;
        } variable_reference;
    };
};

AstNode *ast_create_program(void);
void ast_program_add(AstNode *program, AstNode *statement);

AstNode *ast_create_string(const char *value);
AstNode *ast_create_call(const char *name, AstNode *argument);

AstNode *ast_create_variable_declaration(
    const char *name,
    AstNode *value
);

AstNode *ast_create_variable_reference(const char *name);

void ast_print(AstNode *node, int indent);
void ast_free(AstNode *node);

#endif
