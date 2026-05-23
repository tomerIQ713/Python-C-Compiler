
#include "compiler_common.h"

/*
 * Maps a token type to its translation-table key.
 * O(1).
 */
char* tokenTypeToTranslationKey(TokenType type)
{
    switch (type) {
    case TOKEN_KEYWORD_IF:
        return "TOKEN_KEYWORD_IF";
    case TOKEN_KEYWORD_ELIF:
        return "TOKEN_KEYWORD_ELIF";
    case TOKEN_KEYWORD_ELSE:
        return "TOKEN_KEYWORD_ELSE";
    case TOKEN_KEYWORD_WHILE:
        return "TOKEN_KEYWORD_WHILE";
    case TOKEN_KEYWORD_PRINT:
        return "TOKEN_KEYWORD_PRINT";
    case TOKEN_KEYWORD_DEF:
        return "TOKEN_KEYWORD_DEF";
    case TOKEN_KEYWORD_RETURN:
        return "TOKEN_KEYWORD_RETURN";
    case TOKEN_KEYWORD_TRUE:
        return "TOKEN_KEYWORD_TRUE";
    case TOKEN_KEYWORD_FALSE:
        return "TOKEN_KEYWORD_FALSE";
    case TOKEN_KEYWORD_AND:
        return "TOKEN_KEYWORD_AND";
    case TOKEN_KEYWORD_OR:
        return "TOKEN_KEYWORD_OR";
    case TOKEN_KEYWORD_NOT:
        return "TOKEN_KEYWORD_NOT";

    case TOKEN_OPERATOR_ASSIGN:
        return "TOKEN_OPERATOR_ASSIGN";
    case TOKEN_OPERATOR_PLUS_ASSIGN:
        return "TOKEN_OPERATOR_PLUS_ASSIGN";
    case TOKEN_OPERATOR_MINUS_ASSIGN:
        return "TOKEN_OPERATOR_MINUS_ASSIGN";
    case TOKEN_OPERATOR_EQUAL:
        return "TOKEN_OPERATOR_EQUAL";
    case TOKEN_OPERATOR_NOT_EQUAL:
        return "TOKEN_OPERATOR_NOT_EQUAL";
    case TOKEN_OPERATOR_GTE:
        return "TOKEN_OPERATOR_GTE";
    case TOKEN_OPERATOR_LTE:
        return "TOKEN_OPERATOR_LTE";
    case TOKEN_OPERATOR_GT:
        return "TOKEN_OPERATOR_GT";
    case TOKEN_OPERATOR_LT:
        return "TOKEN_OPERATOR_LT";
    case TOKEN_OPERATOR_PLUS:
        return "TOKEN_OPERATOR_PLUS";
    case TOKEN_OPERATOR_MINUS:
        return "TOKEN_OPERATOR_MINUS";
    case TOKEN_OPERATOR_MULTIPLY:
        return "TOKEN_OPERATOR_MULTIPLY";
    case TOKEN_OPERATOR_DIVIDE:
        return "TOKEN_OPERATOR_DIVIDE";
    case TOKEN_OPERATOR_MODULO:
        return "TOKEN_OPERATOR_MODULO";

    case TOKEN_LPAREN:
        return "TOKEN_LPAREN";
    case TOKEN_RPAREN:
        return "TOKEN_RPAREN";
    case TOKEN_COLON:
        return "TOKEN_COLON";

    default:
        return "";
    }
}

/*
 * Finds the C text for a token.
 * O(lookup).
 */
char* findTokenTranslation(Token token)
{
    char* key = tokenTypeToTranslationKey(token.type);

    if (strlen(key) == 0)
        return NULL;

    return findTranslation(key);
}

/*
 * Finds the C type for an internal type name.
 *  O(n + lookup).
 */
char* findCTypeTranslation(char* type)
{
    char key[MAX_WORD];
    char* result;

    strcpy(key, "ctype:");
    strcat(key, type);

    result = findTranslation(key);

    if (result != NULL)
        return result;

    return type;
}

/*
 * Finds the printf format specifier for a type.
 * O(n + lookup).
 */
char* findPrintfTranslation(char* type)
{
    char key[MAX_WORD];
    char* result;

    strcpy(key, "printf:");
    strcat(key, type);

    result = findTranslation(key);

    if (result != NULL)
        return result;

    return "%d";
}

/*
 * Infers the C type needed for a translated expression.
 * O(k * lookup).
 */
char* inferCTypeFromExpression(TokenLine* line, int start, int end)
{
    for (int i = start; i <= end; i++) {
        if (line->tokens[i].type == TOKEN_STRING)
            return "char*";

        if (line->tokens[i].type == TOKEN_FLOAT_NUMBER)
            return "float";

        if (line->tokens[i].type == TOKEN_IDENTIFIER) {
            SymbolNode* symbol = findSymbol(line->tokens[i].value);

            if (symbol != NULL) {
                if (strcmp(symbol->type, "char*") == 0)
                    return "char*";

                if (strcmp(symbol->type, "float") == 0)
                    return "float";
            }
        }
    }

    return "int";
}