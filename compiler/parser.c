#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void advance(Parser *parser)
{
    parser->previous = parser->current;
    parser->current = lexer_next(parser->lexer);
}

static void parser_error(Parser *parser, const char *message)
{
    fprintf(
        stderr,
        "Surge parser error on line %d: %s\n",
        parser->current.line,
        message
    );

    exit(1);
}

static void consume(
    Parser *parser,
    TokenType type,
    const char *message
)
{
    if (parser->current.type == type)
    {
        advance(parser);
        return;
    }

    parser_error(parser, message);
}

static char *token_to_string(Token token)
{
    char *text = malloc((size_t)token.length + 1);

    if (text == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    memcpy(text, token.start, (size_t)token.length);
    text[token.length] = '\0';

    return text;
}

static AstNode *parse_string(Parser *parser)
{
    int length = parser->previous.length - 2;

    if (length < 0)
    {
        parser_error(parser, "Invalid string literal.");
    }

    char *value = malloc((size_t)length + 1);

    if (value == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    memcpy(
        value,
        parser->previous.start + 1,
        (size_t)length
    );

    value[length] = '\0';

    AstNode *node = ast_create_string(value);

    free(value);

    return node;
}

static AstNode *parse_call(Parser *parser)
{
    char *name = token_to_string(parser->previous);

    consume(
        parser,
        TOKEN_LEFT_PAREN,
        "Expected '(' after function name."
    );

    if (parser->current.type != TOKEN_STRING)
    {
        free(name);

        parser_error(
            parser,
            "Expected a string argument."
        );
    }

    advance(parser);

    AstNode *argument = parse_string(parser);

    consume(
        parser,
        TOKEN_RIGHT_PAREN,
        "Expected ')' after function argument."
    );

    AstNode *node = ast_create_call(name, argument);

    free(name);

    return node;
}

void parser_init(Parser *parser, Lexer *lexer)
{
    parser->lexer = lexer;

    parser->current.type = TOKEN_EOF;
    parser->previous.type = TOKEN_EOF;


    advance(parser);
}

AstNode *parser_parse(Parser *parser)
{
    if (parser->current.type != TOKEN_IDENTIFIER)
    {
        parser_error(
            parser,
            "Expected a function call."
        );
    }

    advance(parser);

    AstNode *node = parse_call(parser);

    if (parser->current.type != TOKEN_EOF)
    {
        parser_error(
            parser,
            "Expected the end of the file."
        );
    }

    return node;
}
