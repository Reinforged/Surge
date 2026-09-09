#ifndef SURGE_LEXER_H
#define SURGE_LEXER_H

typedef enum {
    TOKEN_EOF,

    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH
} TokenType;

typedef struct {
    TokenType type;

    const char *start;
    int length;

    int line;
} Token;

typedef struct {
    const char *start;
    const char *current;

    int line;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next(Lexer *lexer);

#endif
