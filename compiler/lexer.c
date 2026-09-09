#include "lexer.h"

#include <ctype.h>

static int is_at_end(Lexer *lexer)
{
    return *lexer->current == '\0';
}

static char advance_char(Lexer *lexer)
{
    lexer->current++;
    return lexer->current[-1];
}

static char peek_char(Lexer *lexer)
{
    return *lexer->current;
}

static Token make_token(Lexer *lexer, TokenType type)
{
    Token token;

    token.type = type;
    token.start = lexer->start;
    token.length = (int)(lexer->current - lexer->start);
    token.line = lexer->line;

    return token;
}

static void skip_whitespace(Lexer *lexer)
{
    while (1)
    {
        char c = peek_char(lexer);

        if (c == ' ' || c == '\t' || c == '\r')
        {
            advance_char(lexer);
        }
        else if (c == '\n')
        {
            lexer->line++;
            advance_char(lexer);
        }
        else
        {
            return;
        }
    }
}

static Token string_token(Lexer *lexer)
{
    while (!is_at_end(lexer) && peek_char(lexer) != '"')
    {
        if (peek_char(lexer) == '\n')
        {
            lexer->line++;
        }

        advance_char(lexer);
    }

    if (!is_at_end(lexer))
    {
        advance_char(lexer);
    }

    return make_token(lexer, TOKEN_STRING);
}

static Token identifier_token(Lexer *lexer)
{
    while (
        isalnum((unsigned char)peek_char(lexer)) ||
        peek_char(lexer) == '_'
    )
    {
        advance_char(lexer);
    }

    return make_token(lexer, TOKEN_IDENTIFIER);
}

void lexer_init(Lexer *lexer, const char *source)
{
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
}

Token lexer_next(Lexer *lexer)
{
    skip_whitespace(lexer);

    lexer->start = lexer->current;

    if (is_at_end(lexer))
    {
        return make_token(lexer, TOKEN_EOF);
    }

    char c = advance_char(lexer);

    if (isalpha((unsigned char)c) || c == '_')
    {
        return identifier_token(lexer);
    }

    switch (c)
    {
        case '(':
            return make_token(lexer, TOKEN_LEFT_PAREN);

        case ')':
            return make_token(lexer, TOKEN_RIGHT_PAREN);

        case '"':
            return string_token(lexer);
    }

    return make_token(lexer, TOKEN_EOF);
}
