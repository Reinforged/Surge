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

static AstNode *parse_integer(Parser *parser)
{
    char *text = token_to_string(parser->previous);

    long value = strtol(text, NULL, 10);

    free(text);

    return ast_create_integer(value);
}

static AstNode *parse_primary(Parser *parser)
{
    if (parser->current.type == TOKEN_STRING)
    {
        advance(parser);
        return parse_string(parser);
    }

    if (parser->current.type == TOKEN_NUMBER)
    {
        advance(parser);
        return parse_integer(parser);
    }

    if (parser->current.type == TOKEN_IDENTIFIER)
    {
        advance(parser);

        char *name = token_to_string(parser->previous);
        AstNode *node = ast_create_variable_reference(name);
        free(name);

        return node;
    }

    parser_error(parser, "Expected an expression.");
    return NULL;
}

static AstNode *parse_expression(Parser *parser)
{
    AstNode *left = parse_primary(parser);

    while (parser->current.type == TOKEN_PLUS)
    {
        TokenType operator = parser->current.type;
        advance(parser);

        AstNode *right = parse_primary(parser);

        left = ast_create_binary(left, operator, right);
    }

    return left;
}

static AstNode *parse_variable_declaration(Parser *parser)
{
    Token name_token = parser->previous;
    char *name = token_to_string(name_token);

    consume(
        parser,
        TOKEN_EQUAL,
        "Expected '=' after variable name."
    );

    AstNode *value = parse_expression(parser);

    AstNode *declaration =
        ast_create_variable_declaration(name, value);

    free(name);

    return declaration;
}

static AstNode *parse_call(Parser *parser)
{
    char *name = token_to_string(parser->previous);

    consume(
        parser,
        TOKEN_LEFT_PAREN,
        "Expected '(' after function name."
    );

    AstNode *argument = NULL;

    if (parser->current.type == TOKEN_STRING)
    {
        advance(parser);
        argument = parse_string(parser);
    }
    else if (parser->current.type == TOKEN_IDENTIFIER)
    {
        advance(parser);

        char *variable_name =
            token_to_string(parser->previous);

        argument =
            ast_create_variable_reference(variable_name);

        free(variable_name);
    }
    else
    {
        free(name);
        parser_error(
            parser,
            "Expected a string or variable."
        );
    }

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
    AstNode *program = ast_create_program();

    while (parser->current.type != TOKEN_EOF)
    {
        if (parser->current.type != TOKEN_IDENTIFIER)
        {
            parser_error(parser, "Expected a statement.");
        }

        advance(parser);

        AstNode *statement = NULL;

        if (parser->current.type == TOKEN_EQUAL)
        {
            statement = parse_variable_declaration(parser);
        }
        else if (parser->current.type == TOKEN_LEFT_PAREN)
        {
            statement = parse_call(parser);
        }
        else
        {
            parser_error(
                parser,
                "Expected '=' or '(' after identifier."
            );
        }

        ast_program_add(program, statement);
    }

    return program;
}
