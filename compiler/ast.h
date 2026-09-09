#ifndef SURGE_AST_H
#define SURGE_AST_H

typedef enum {
    AST_STRING,
    AST_CALL
} AstNodeType;

typedef struct AstNode AstNode;

struct AstNode {
    AstNodeType type;

    union {
        struct {
            char *value;
        } string;

        struct {
            char *name;
            AstNode *argument;
        } call;
    };
};

AstNode *ast_create_string(const char *value);
AstNode *ast_create_call(const char *name, AstNode *argument);

void ast_print(AstNode *node, int indent);
void ast_free(AstNode *node);

#endif
