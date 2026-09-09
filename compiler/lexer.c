#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

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
            lexer->at_line_start = 1;
            return;
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

    int length = (int)(lexer->current - lexer->start);

    if (length == 2 &&
        lexer->start[0] == 'i' &&
        lexer->start[1] == 'f')
    {
        return make_token(lexer, TOKEN_IF);
    }

    return make_token(lexer, TOKEN_IDENTIFIER);
}

static Token number_token(Lexer *lexer)
{
    while (isdigit((unsigned char)peek_char(lexer)))
    {
        advance_char(lexer);
    }

    return make_token(lexer, TOKEN_NUMBER);
}

void lexer_init(Lexer *lexer, const char *source)
{
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->indent_stack[0] = 0;
    lexer->indent_count = 1;
    lexer->pending_dedents = 0;
    lexer->at_line_start = 1;
}

static int match_char(Lexer *lexer, char expected)
{
    if (is_at_end(lexer) || *lexer->current != expected)
    {
        return 0;
    }

    lexer->current++;
    return 1;
}

Token lexer_next(Lexer *lexer);

static Token handle_indentation(Lexer *lexer)
{
    int spaces = 0;

    while (peek_char(lexer) == ' ')
    {
        advance_char(lexer);
        spaces++;
    }

    if (peek_char(lexer) == '\n' || is_at_end(lexer))
    {
        lexer->at_line_start = 0;
        return lexer_next(lexer);
    }

    int current_indent =
        lexer->indent_stack[lexer->indent_count - 1];

    if (spaces > current_indent)
    {
        if (lexer->indent_count >= 64)
        {
            fprintf(
                stderr,
                "Surge lexer error: maximum indentation depth exceeded.\n"
            );
            exit(1);
        }

        lexer->indent_stack[lexer->indent_count] = spaces;
        lexer->indent_count++;
        lexer->at_line_start = 0;

        return make_token(lexer, TOKEN_INDENT);
    }

    if (spaces < current_indent)
    {
        int target_count = lexer->indent_count;

        while (
            target_count > 1 &&
            spaces < lexer->indent_stack[target_count - 1]
        )
        {
            target_count--;
        }

        if (spaces != lexer->indent_stack[target_count - 1])
        {
            fprintf(
                stderr,
                "Surge lexer error: inconsistent indentation.\n"
            );
            exit(1);
        }

        lexer->pending_dedents =
            lexer->indent_count - target_count;

        lexer->indent_count = target_count;
        lexer->at_line_start = 0;

        lexer->pending_dedents--;

        return make_token(lexer, TOKEN_DEDENT);
    }

    lexer->at_line_start = 0;
    return lexer_next(lexer);
}

Token lexer_next(Lexer *lexer)
{
    if (lexer->pending_dedents > 0)
    {
        lexer->start = lexer->current;
        lexer->pending_dedents--;
        return make_token(lexer, TOKEN_DEDENT);
    }

    if (lexer->at_line_start)
    {
        lexer->start = lexer->current;
        return handle_indentation(lexer);
    }

    skip_whitespace(lexer);

    if (lexer->at_line_start)
    {
        lexer->start = lexer->current;
        return handle_indentation(lexer);
    }

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
    
    if (isdigit((unsigned char)c))
    {
        return number_token(lexer);
    }

    switch (c)
    {
        case '(':
            return make_token(lexer, TOKEN_LEFT_PAREN);

        case ')':
            return make_token(lexer, TOKEN_RIGHT_PAREN);

        case '"':
            return string_token(lexer);
            
        case '=':
            return make_token(
                lexer,
                match_char(lexer, '=') ?
                    TOKEN_EQUAL_EQUAL :
                    TOKEN_EQUAL
            );
            
        case '!':
            return make_token(
                lexer,
                match_char(lexer, '=') ?
                    TOKEN_BANG_EQUAL :
                    TOKEN_EOF
            );

        case '<':
            return make_token(
                lexer,
                match_char(lexer, '=') ?
                    TOKEN_LESS_EQUAL :
                    TOKEN_LESS
            );

        case '>':
            return make_token(
                lexer,
                match_char(lexer, '=') ?
                    TOKEN_GREATER_EQUAL :
                    TOKEN_GREATER
            );
            
        case '+':
            return make_token(lexer, TOKEN_PLUS);
            
        case '-':
            return make_token(lexer, TOKEN_MINUS);

        case '*':
            return make_token(lexer, TOKEN_STAR);

        case '/':
            return make_token(lexer, TOKEN_SLASH);
    }

    return make_token(lexer, TOKEN_EOF);
}
