#include <stdio.h>

#include "ast.h"

int main(void)
{
    AstNode *message = ast_create_string("Hello, Surge!");

    AstNode *program = ast_create_call(
        "print",
        message
    );

    ast_print(program, 0);

    ast_free(program);

    return 0;
}
