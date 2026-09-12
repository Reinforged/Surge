#include "interpreter.h"
#include "value.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    Value value;
} Variable;

typedef struct {
    char *name;
    char **parameters;
    int parameter_count;
    AstNode *body;
} Function;

typedef struct {
    int returned;
    int broke;
    int continued;
    Value value;
} ExecutionResult;

typedef struct Environment Environment;

struct Environment {
    Variable *variables;
    int count;
    Environment *parent;
    Function *functions;
    int function_count;
};

static Environment *environment_create(Environment *parent)
{
    Environment *environment = malloc(sizeof(Environment));

    if (environment == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }
    
    environment->functions = NULL;
    environment->function_count = 0;
    environment->variables = NULL;
    environment->count = 0;
    environment->parent = parent;

    return environment;
}

static Variable *find_variable(Environment *environment, const char *name)
{
    for (Environment *current = environment;
         current != NULL;
         current = current->parent)
    {
        for (int i = 0; i < current->count; i++)
        {
            if (strcmp(current->variables[i].name, name) == 0)
            {
                return &current->variables[i];
            }
        }
    }

    return NULL;
}

static Function *find_function(
    Environment *environment,
    const char *name
)
{
    for (Environment *current = environment;
         current != NULL;
         current = current->parent)
    {
        for (int i = 0; i < current->function_count; i++)
        {
            if (strcmp(current->functions[i].name, name) == 0)
            {
                return &current->functions[i];
            }
        }
    }

    return NULL;
}

static void set_variable(
    Environment *environment,
    const char *name,
    Value value
)
{
    Variable *variable = find_variable(environment, name);

    if (variable != NULL)
    {
        value_free(&variable->value);
        variable->value = value;
        return;
    }

    Variable *variables = realloc(
        environment->variables,
        sizeof(Variable) * (environment->count + 1)
    );

    if (variables == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    environment->variables = variables;

    Variable *new_variable =
        &environment->variables[environment->count];

    new_variable->name = malloc(strlen(name) + 1);

    if (new_variable->name == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    strcpy(new_variable->name, name);
    new_variable->value = value;

    environment->count++;
}

static void environment_free(Environment *environment)
{
    for (int i = 0; i < environment->count; i++)
    {
        free(environment->variables[i].name);
        value_free(&environment->variables[i].value);
    }
    
    for (int i = 0; i < environment->function_count; i++)
    {
        free(environment->functions[i].name);

        for (int j = 0;
             j < environment->functions[i].parameter_count;
             j++)
        {
            free(environment->functions[i].parameters[j]);
        }

        free(environment->functions[i].parameters);
    }

    free(environment->variables);
    free(environment->functions);
    free(environment);
}

static void define_function(
    Environment *environment,
    const char *name,
    char **parameters,
    int parameter_count,
    AstNode *body
)
{
    Function *function = find_function(environment, name);

    if (function != NULL)
    {
        function->body = body;
        return;
    }

    Function *functions = realloc(
        environment->functions,
        sizeof(Function) * (environment->function_count + 1)
    );

    if (functions == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    environment->functions = functions;

    Function *new_function =
        &environment->functions[environment->function_count];

    new_function->name = malloc(strlen(name) + 1);

    if (new_function->name == NULL)
    {
        fprintf(stderr, "Surge: out of memory.\n");
        exit(1);
    }

    strcpy(new_function->name, name);

    new_function->parameter_count = parameter_count;

    new_function->parameters = NULL;

    if (parameter_count > 0)
    {
        new_function->parameters = malloc(
            sizeof(char *) * parameter_count
        );

        if (new_function->parameters == NULL)
        {
            fprintf(stderr, "Surge: out of memory.\n");
            exit(1);
        }
    }
    
    for (int i = 0; i < parameter_count; i++)
    {
        new_function->parameters[i] =
            malloc(strlen(parameters[i]) + 1);

        if (new_function->parameters[i] == NULL)
        {
            fprintf(stderr, "Surge: out of memory.\n");
            exit(1);
        }

        strcpy(
            new_function->parameters[i],
            parameters[i]
        );
    }

    new_function->body = body;
    environment->function_count++;
}

static ExecutionResult execute(
    AstNode *node,
    Environment *environment
);

static Value evaluate(
    AstNode *node,
    Environment *environment
);

static Value *resolve_array_element(
    AstNode *node,
    Environment *environment
)
{
    if (node->type == AST_VARIABLE_REFERENCE)
    {
        Variable *variable = find_variable(
            environment,
            node->variable_reference.name
        );

        if (variable == NULL)
        {
            fprintf(
                stderr,
                "Surge runtime error: variable '%s' is not defined.\n",
                node->variable_reference.name
            );
            exit(1);
        }

        return &variable->value;
    }

    if (node->type == AST_INDEX)
    {
        Value *array = resolve_array_element(
            node->index.array,
            environment
        );

        Value index = evaluate(node->index.index, environment);

        if (array->type != VALUE_ARRAY)
        {
            value_free(&index);
            fprintf(
                stderr,
                "Surge runtime error: indexing requires an array.\n"
            );
            exit(1);
        }

        if (index.type != VALUE_INT)
        {
            value_free(&index);
            fprintf(
                stderr,
                "Surge runtime error: array index must be an integer.\n"
            );
            exit(1);
        }

        if (index.integer < 0 || index.integer >= array->array.count)
        {
            value_free(&index);
            fprintf(
                stderr,
                "Surge runtime error: array index out of bounds.\n"
            );
            exit(1);
        }

        Value *element = &array->array.elements[index.integer];
        value_free(&index);
        return element;
    }

    fprintf(
        stderr,
        "Surge runtime error: array assignment requires an array expression.\n"
    );
    exit(1);
}

static Value evaluate_index(
    AstNode *array_node,
    AstNode *index_node,
    Environment *environment
)
{
    Value array = evaluate(array_node, environment);
    Value index = evaluate(index_node, environment);

    if (array.type != VALUE_ARRAY)
    {
        value_free(&array);
        value_free(&index);
        fprintf(stderr, "Surge runtime error: indexing requires an array.\n");
        exit(1);
    }

    if (index.type != VALUE_INT)
    {
        value_free(&array);
        value_free(&index);
        fprintf(stderr, "Surge runtime error: array index must be an integer.\n");
        exit(1);
    }

    if (index.integer < 0 || index.integer >= array.array.count)
    {
        value_free(&array);
        value_free(&index);
        fprintf(stderr, "Surge runtime error: array index out of bounds.\n");
        exit(1);
    }

    Value result = value_copy(&array.array.elements[index.integer]);
    value_free(&array);
    value_free(&index);
    return result;
}

static Value evaluate(
    AstNode *node,
    Environment *environment
)
{
    if (node->type == AST_STRING)
    {
        return value_string(node->string.value);
    }

    if (node->type == AST_INTEGER)
    {
        return value_int(node->integer.value);
    }
    
    if (node->type == AST_BOOLEAN)
    {
        return value_bool(node->boolean.value);
    }

    if (node->type == AST_ARRAY)
    {
        Value array = value_array(node->array.count);

        for (int i = 0; i < node->array.count; i++)
        {
            array.array.elements[i] = evaluate(
                node->array.elements[i],
                environment
            );
        }

        return array;
    }

    if (node->type == AST_INDEX)
    {
        return evaluate_index(
            node->index.array,
            node->index.index,
            environment
        );
    }
    
    if (node->type == AST_UNARY)
    {
        Value operand = evaluate(
            node->unary.operand,
            environment
        );

        switch (node->unary.operator)
        {
            case TOKEN_NOT:
                if (operand.type != VALUE_BOOL)
                {
                    fprintf(
                        stderr,
                        "Surge runtime error: 'not' requires a boolean value.\n"
                    );
                    exit(1);
                }

                return value_bool(!operand.boolean);

            case TOKEN_MINUS:
                if (operand.type != VALUE_INT)
                {
                    fprintf(
                        stderr,
                        "Surge runtime error: unary '-' requires an integer value.\n"
                    );
                    exit(1);
                }

                return value_int(-operand.integer);

            default:
                fprintf(
                    stderr,
                    "Surge runtime error: unknown unary operator.\n"
                );
                exit(1);
        }
    }

    if (node->type == AST_VARIABLE_REFERENCE)
    {
        Variable *variable = find_variable(
            environment,
            node->variable_reference.name
        );

        if (variable == NULL)
        {
            fprintf(
                stderr,
                "Surge runtime error: variable '%s' is not defined.\n",
                node->variable_reference.name
            );
            exit(1);
        }

        return value_copy(&variable->value);
    }

    if (node->type == AST_CALL)
    {
        if (strcmp(node->call.name, "print") == 0)
        {
            for (int i = 0; i < node->call.argument_count; i++)
            {
                Value argument = evaluate(
                    node->call.arguments[i],
                    environment
                );

                value_print(&argument);

                value_free(&argument);
            }

            return value_int(0);
        }

        Function *function = find_function(
            environment,
            node->call.name
        );

        if (function == NULL)
        {
            fprintf(
                stderr,
                "Surge runtime error: function '%s' is not defined.\n",
                node->call.name
            );
            exit(1);
        }

        if (function->parameter_count != node->call.argument_count)
        {
            fprintf(
                stderr,
                "Surge runtime error: function '%s' expects %d argument(s), got %d.\n",
                node->call.name,
                function->parameter_count,
                node->call.argument_count
            );
            exit(1);
        }

        Environment *function_environment =
            environment_create(environment);

        for (int i = 0; i < function->parameter_count; i++)
        {
            Value argument = evaluate(
                node->call.arguments[i],
                environment
            );

            set_variable(
                function_environment,
                function->parameters[i],
                argument
            );
        }

        ExecutionResult function_result = execute(
            function->body,
            function_environment
        );

        environment_free(function_environment);

        if (function_result.broke || function_result.continued)
        {
            fprintf(
                stderr,
                "Surge runtime error: loop control statement outside of a loop.\n"
            );
            exit(1);
        }

        return function_result.value;
    }

    if (node->type == AST_BINARY)
    {
        if (node->binary.operator == TOKEN_AND)
        {
            Value left = evaluate(
                node->binary.left,
                environment
            );

            if (left.type != VALUE_BOOL)
            {
                fprintf(
                    stderr,
                    "Surge runtime error: 'and' requires boolean values.\n"
                );
                exit(1);
            }

            if (!left.boolean)
            {
                return value_bool(0);
            }

            Value right = evaluate(
                node->binary.right,
                environment
            );

            if (right.type != VALUE_BOOL)
            {
                fprintf(
                    stderr,
                    "Surge runtime error: 'and' requires boolean values.\n"
                );
                exit(1);
            }

            return value_bool(right.boolean);
        }

        if (node->binary.operator == TOKEN_OR)
        {
            Value left = evaluate(
                node->binary.left,
                environment
            );

            if (left.type != VALUE_BOOL)
            {
                fprintf(
                    stderr,
                    "Surge runtime error: 'or' requires boolean values.\n"
                );
                exit(1);
            }

            if (left.boolean)
            {
                return value_bool(1);
            }

            Value right = evaluate(
                node->binary.right,
                environment
            );

            if (right.type != VALUE_BOOL)
            {
                fprintf(
                    stderr,
                    "Surge runtime error: 'or' requires boolean values.\n"
                );
                exit(1);
            }

            return value_bool(right.boolean);
        }

        Value left = evaluate(
            node->binary.left,
            environment
        );

        Value right = evaluate(
            node->binary.right,
            environment
        );

        if (left.type == VALUE_STRING &&
            right.type == VALUE_STRING)
        {
            switch (node->binary.operator)
            {
                case TOKEN_PLUS:
                {
                    size_t left_length = strlen(left.string);
                    size_t right_length = strlen(right.string);

                    char *result = malloc(
                        left_length + right_length + 1
                    );

                    if (result == NULL)
                    {
                        fprintf(stderr, "Surge: out of memory.\n");
                        exit(1);
                    }

                    memcpy(result, left.string, left_length);
                    memcpy(
                        result + left_length,
                        right.string,
                        right_length + 1
                    );

                    Value value = value_string(result);
                    free(result);
                    value_free(&left);
                    value_free(&right);
                    return value;
                }

                case TOKEN_EQUAL_EQUAL:
                {
                    int equal = strcmp(left.string, right.string) == 0;
                    value_free(&left);
                    value_free(&right);
                    return value_bool(equal);
                }

                case TOKEN_BANG_EQUAL:
                {
                    int not_equal = strcmp(left.string, right.string) != 0;
                    value_free(&left);
                    value_free(&right);
                    return value_bool(not_equal);
                }

                default:
                    value_free(&left);
                    value_free(&right);
                    fprintf(
                        stderr,
                        "Surge runtime error: unsupported string operator.\n"
                    );
                    exit(1);
            }
        }

        if (left.type != VALUE_INT || right.type != VALUE_INT)
        {
            value_free(&left);
            value_free(&right);
            fprintf(
                stderr,
                "Surge runtime error: arithmetic and numeric comparisons require integer values.\n"
            );
            exit(1);
        }

        switch (node->binary.operator)
        {
            case TOKEN_PLUS:
                return value_int(left.integer + right.integer);

            case TOKEN_MINUS:
                return value_int(left.integer - right.integer);

            case TOKEN_STAR:
                return value_int(left.integer * right.integer);

            case TOKEN_SLASH:
                if (right.integer == 0)
                {
                    fprintf(
                        stderr,
                        "Surge runtime error: division by zero.\n"
                    );
                    exit(1);
                }

                return value_int(left.integer / right.integer);

            case TOKEN_PERCENT:
                if (right.integer == 0)
                {
                    fprintf(
                        stderr,
                        "Surge runtime error: modulo by zero.\n"
                    );
                    exit(1);
                }

                return value_int(left.integer % right.integer);

            case TOKEN_EQUAL_EQUAL:
                return value_bool(left.integer == right.integer);

            case TOKEN_BANG_EQUAL:
                return value_bool(left.integer != right.integer);

            case TOKEN_LESS:
                return value_bool(left.integer < right.integer);

            case TOKEN_LESS_EQUAL:
                return value_bool(left.integer <= right.integer);

            case TOKEN_GREATER:
                return value_bool(left.integer > right.integer);

            case TOKEN_GREATER_EQUAL:
                return value_bool(left.integer >= right.integer);

            default:
                fprintf(
                    stderr,
                    "Surge runtime error: unknown binary operator.\n"
                );
                exit(1);
        }
    }

    fprintf(
        stderr,
        "Surge runtime error: invalid expression.\n"
    );
    exit(1);
}

static ExecutionResult execute(
    AstNode *node,
    Environment *environment
)
{
    
    ExecutionResult result = {
        0,
        0,
        0,
        value_int(0)
    };
    
    if (node == NULL)
    {
        return result;
    }

    switch (node->type)
    {
        case AST_PROGRAM:
            for (int i = 0; i < node->program.count; i++)
            {
                result = execute(
                    node->program.statements[i],
                    environment
                );

                if (result.returned || result.broke || result.continued)
                {
                    return result;
                }
            }
            break;

        case AST_VARIABLE_DECLARATION:
        {
            Value value = evaluate(
                node->variable_declaration.value,
                environment
            );

            set_variable(
                environment,
                node->variable_declaration.name,
                value
            );

            break;
        }

        case AST_CALL:
        {
            Value value = evaluate(node, environment);

            value_free(&value);

            break;
        }

        case AST_STRING:
        case AST_INTEGER:
        case AST_BOOLEAN:
        case AST_VARIABLE_REFERENCE:
        case AST_BINARY:
        case AST_UNARY:
        case AST_ARRAY:
        case AST_INDEX:
            break;
            
        case AST_INDEX_ASSIGNMENT:
        {
            Value *array = resolve_array_element(
                node->index_assignment.array,
                environment
            );
            Value index = evaluate(
                node->index_assignment.index,
                environment
            );
            Value value = evaluate(
                node->index_assignment.value,
                environment
            );

            if (array->type != VALUE_ARRAY)
            {
                value_free(&index);
                value_free(&value);
                fprintf(stderr, "Surge runtime error: indexing requires an array.\n");
                exit(1);
            }

            if (index.type != VALUE_INT)
            {
                value_free(&index);
                value_free(&value);
                fprintf(stderr, "Surge runtime error: array index must be an integer.\n");
                exit(1);
            }

            if (index.integer < 0 || index.integer >= array->array.count)
            {
                value_free(&index);
                value_free(&value);
                fprintf(stderr, "Surge runtime error: array index out of bounds.\n");
                exit(1);
            }

            value_free(&array->array.elements[index.integer]);
            array->array.elements[index.integer] = value;
            value_free(&index);
            break;
        }

        case AST_FUNCTION:
            define_function(
                environment,
                node->function.name,
                node->function.parameters,
                node->function.parameter_count,
                node->function.body
            );
            break;
            
        case AST_RETURN:
        {
            result.value = evaluate(
                node->return_statement.value,
                environment
            );

            result.returned = 1;

            return result;
        }

        case AST_IF:
        {
            Value condition = evaluate(
                node->if_statement.condition,
                environment
            );

            if (condition.type != VALUE_BOOL)
            {
                fprintf(
                    stderr,
                    "Surge runtime error: if condition must be a boolean.\n"
                );
                exit(1);
            }

            if (condition.boolean)
            {
                Environment *block =
                    environment_create(environment);

                result = execute(node->if_statement.body, block);
                environment_free(block);

                if (result.returned || result.broke || result.continued)
                {
                    return result;
                }
            }
            else if (node->if_statement.else_body != NULL)
            {
                Environment *block =
                    environment_create(environment);

                result = execute(node->if_statement.else_body, block);
                environment_free(block);

                if (result.returned || result.broke || result.continued)
                {
                    return result;
                }
            }

            break;
        }
            
        case AST_WHILE:
        {
            while (1)
            {
                Value condition = evaluate(
                    node->while_statement.condition,
                    environment
                );

                if (condition.type != VALUE_BOOL)
                {
                    fprintf(
                        stderr,
                        "Surge runtime error: while condition must be a boolean.\n"
                    );
                    exit(1);
                }

                if (!condition.boolean)
                {
                    break;
                }

                Environment *block =
                    environment_create(environment);

                result = execute(node->while_statement.body, block);

                environment_free(block);

                if (result.returned)
                {
                    return result;
                }

                if (result.broke)
                {
                    result.broke = 0;
                    return result;
                }

                if (result.continued)
                {
                    result.continued = 0;
                    continue;
                }
            }

            break;
        }

        case AST_BREAK:
            result.broke = 1;
            return result;

        case AST_CONTINUE:
            result.continued = 1;
            return result;
    }

    return result;
}

void interpreter_run(AstNode *program)
{
    Environment *environment =
        environment_create(NULL);

    ExecutionResult result = execute(program, environment);

    environment_free(environment);

    if (result.broke || result.continued)
    {
        fprintf(
            stderr,
            "Surge runtime error: loop control statement outside of a loop.\n"
        );
        exit(1);
    }
}
