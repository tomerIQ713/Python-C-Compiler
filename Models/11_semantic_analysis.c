#include "compiler_common.h"
#define EOD '\0'

/*
 * Semantic Analysis Module
 * ------------------------
 * Responsible for checking semantic correctness of the Python source code.
 * It validates:
 *  - variables defined before usage
 *  - variables initialized before usage (important for if/while/for blocks)
 *  - function definitions and calls
 *  - constant reassignment
 *  - arithmetic and comparison type correctness
 *
 * Time Complexity:
 * O(n * t) per line, where:
 *   n = number of tokens in the line
 *   t = size of symbol/function tables (linear search)
 */


 /* ================================
    FUNCTION TABLE (FOR TYPE CHECK)
    ================================ */

typedef struct {
    char name[MAX_WORD];
    char returnType[20];
    char paramTypes[MAX_PARAMS][20];
    int paramCount;
} FunctionInfo;

FunctionInfo functionTable[MAX_FUNCTIONS];
int functionCount = 0;

char currentFunctionName[MAX_WORD] = "";


/* ================================
   INITIALIZATION TRACKING
   ================================ */

   /*
    * Tracks variable initialization per indentation level.
    * Example:
    * if True:
    *     x = 5   (indentLevel = 4)
    * print(x)    (indentLevel = 0) -> ERROR, x not initialized globally
    */

typedef struct {
    char name[MAX_WORD];
    int indentLevelDefined;
    char scope[MAX_WORD];
} InitVar;

InitVar initializedVars[500];
int initializedCount = 0;


/*
 * Checks if a variable is already marked initialized.
 * O(m), m = initializedCount
 */
int isInitialized(char* name, char* scope, int currentIndent)
{
    for (int i = 0; i < initializedCount; i++) {
        if (strcmp(initializedVars[i].name, name) == 0 &&
            strcmp(initializedVars[i].scope, scope) == 0) {

            return initializedVars[i].indentLevelDefined <= currentIndent;
        }
    }

    return 0;
}


/*
 * Marks variable as initialized in a certain scope at a given indent.
 * O(m), m = initializedCount
 */
void markInitialized(char* name, char* scope, int indentLevel)
{
    for (int i = 0; i < initializedCount; i++) {
        if (strcmp(initializedVars[i].name, name) == 0 &&
            strcmp(initializedVars[i].scope, scope) == 0) {

            if (indentLevel < initializedVars[i].indentLevelDefined)
                initializedVars[i].indentLevelDefined = indentLevel;

            return;
        }
    }

    strcpy(initializedVars[initializedCount].name, name);
    strcpy(initializedVars[initializedCount].scope, scope);
    initializedVars[initializedCount].indentLevelDefined = indentLevel;

    initializedCount++;
}


/* ================================
   HELPER FUNCTIONS
   ================================ */

int isConstantName(char* name)
{
    for (int i = 0; name[i] != '\0'; i++) {
        if (!(name[i] >= 'A' && name[i] <= 'Z') &&
            !(name[i] >= '0' && name[i] <= '9') &&
            name[i] != '_') {
            return 0;
        }
    }
    return 1;
}


int isNumericType(char* type)
{
    return strcmp(type, "int") == 0 || strcmp(type, "float") == 0;
}


char* pythonTypeToCType(TokenType tokenType)
{
    if (tokenType == TOKEN_KEYWORD_INT)
        return "int";

    if (tokenType == TOKEN_KEYWORD_FLOAT)
        return "float";

    if (tokenType == TOKEN_KEYWORD_STR)
        return "char*";

    return "int";
}


/*
 * Finds a function info object by name.
 * O(f), f = number of defined functions
 */
FunctionInfo* findFunctionInfo(char* name)
{
    for (int i = 0; i < functionCount; i++) {
        if (strcmp(functionTable[i].name, name) == 0)
            return &functionTable[i];
    }

    return NULL;
}


/*
 * Adds a function to function table.
 * O(f)
 */
FunctionInfo* addFunctionInfo(char* name)
{
    FunctionInfo* existing = findFunctionInfo(name);

    if (existing != NULL) {
        existing->paramCount = 0;
        strcpy(existing->returnType, "int");
        return existing;
    }

    if (functionCount >= MAX_FUNCTIONS)
        return NULL;

    strcpy(functionTable[functionCount].name, name);
    strcpy(functionTable[functionCount].returnType, "int");
    functionTable[functionCount].paramCount = 0;

    functionCount++;

    return &functionTable[functionCount - 1];
}


/*
 * Updates return type for a function.
 * O(f)
 */
void updateFunctionReturnType(char* name, char* returnType)
{
    FunctionInfo* func = findFunctionInfo(name);

    if (func != NULL)
        strcpy(func->returnType, returnType);
}


/*
 * Returns the semantic type of a single argument token.
 * O(lookup)
 */
char* getArgumentType(Token token)
{
    if (token.type == TOKEN_NUMBER)
        return "int";

    if (token.type == TOKEN_FLOAT_NUMBER)
        return "float";

    if (token.type == TOKEN_STRING)
        return "char*";

    if (token.type == TOKEN_IDENTIFIER) {
        SymbolNode* symbol = findSymbol(token.value);
        if (symbol != NULL)
            return symbol->type;
    }

    return "int";
}


/*
 * Infers the resulting type of an expression.
 * O(n * lookup)
 */
char* inferSemanticExpressionType(Token tokens[], int start, int end)
{
    for (int i = start; i <= end; i++) {
        if (tokens[i].type == TOKEN_STRING)
            return "char*";

        if (tokens[i].type == TOKEN_FLOAT_NUMBER)
            return "float";

        if (tokens[i].type == TOKEN_IDENTIFIER &&
            i + 1 <= end &&
            tokens[i + 1].type == TOKEN_LPAREN) {

            FunctionInfo* funcInfo = findFunctionInfo(tokens[i].value);

            if (funcInfo != NULL)
                return funcInfo->returnType;
        }

        if (tokens[i].type == TOKEN_IDENTIFIER) {
            SymbolNode* symbol = findSymbol(tokens[i].value);

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


/*
 * Checks that all identifiers are defined in symbol table.
 * O(n * lookup)
 */
int checkIdentifiersDefined(Token tokens[], int start, int end, int lineNumber)
{
    for (int i = start; i <= end; i++) {
        if (tokens[i].type == TOKEN_IDENTIFIER) {
            if (findSymbol(tokens[i].value) == NULL) {
                printf("  Semantic error: variable '%s' used before definition\n", tokens[i].value);
                addErrorCode(lineNumber, ERROR_SEMANTIC_UNDEFINED_VARIABLE);
                return ERROR_SEMANTIC_UNDEFINED_VARIABLE;
            }
        }
    }

    return SUCCESS_CODE;
}


/*
 * Checks that identifiers are initialized before use.
 * O(n * lookup + initCount)
 */
int checkIdentifiersInitialized(Token tokens[], int start, int end, int lineNumber, int indentLevel)
{
    char* scope = getCurrentScope();

    for (int i = start; i <= end; i++) {
        if (tokens[i].type == TOKEN_IDENTIFIER) {

            SymbolNode* sym = findSymbol(tokens[i].value);

            if (sym == NULL)
                continue;

            if (i + 1 <= end && tokens[i + 1].type == TOKEN_LPAREN)
                continue;


            if (!isInitialized(tokens[i].value, scope, indentLevel)) {
                printf(
                    "  Semantic error: variable '%s' may be uninitialized when used\n",
                    tokens[i].value
                );

                addErrorCode(lineNumber, ERROR_SEMANTIC_UNINITIALIZED_VARIABLE);
                return ERROR_SEMANTIC_UNINITIALIZED_VARIABLE;
            }
        }
    }

    return SUCCESS_CODE;
}


/*
 * Checks invalid arithmetic type usage.
 * O(n * lookup)
 */
int checkInvalidArithmeticTypes(Token tokens[], int start, int end, int lineNumber)
{
    for (int i = start; i <= end; i++) {
        if (tokens[i].type == TOKEN_OPERATOR_PLUS ||
            tokens[i].type == TOKEN_OPERATOR_MINUS ||
            tokens[i].type == TOKEN_OPERATOR_MULTIPLY ||
            tokens[i].type == TOKEN_OPERATOR_DIVIDE ||
            tokens[i].type == TOKEN_OPERATOR_MODULO) {

            char* leftType = getArgumentType(tokens[i - 1]);
            char* rightType = getArgumentType(tokens[i + 1]);

            if (tokens[i].type == TOKEN_OPERATOR_PLUS &&
                strcmp(leftType, "char*") == 0 &&
                strcmp(rightType, "int") == 0) {
                continue;
            }

            if ((strcmp(leftType, "char*") != 0 && strcmp(rightType, "char*") == 0) ||
                 (strcmp(leftType, "char*") == 0 && strcmp(rightType, "char*") != 0)){
                printf(
                    "  Semantic error: invalid arithmetic operation %s %s %s\n",
                    leftType,
                    tokens[i].value,
                    rightType
                );

                addErrorCode(lineNumber, ERROR_SEMANTIC_INVALID_IDENTIFIER_USE);
                return ERROR_SEMANTIC_INVALID_IDENTIFIER_USE;
            }
        }
    }

    return SUCCESS_CODE;
}



int semanticCheckExpression(Token tokens[], int start, int end, int lineNumber);







int semanticCheckFunctionCallTokens(Token tokens[], int start, int end, int lineNumber)
{
    FunctionInfo* func = findFunctionInfo(tokens[start].value);

    if (func == NULL) {
        printf("  Semantic error: function '%s' used before definition\n", tokens[start].value);
        addErrorCode(lineNumber, ERROR_SEMANTIC_UNDEFINED_VARIABLE);
        return ERROR_SEMANTIC_UNDEFINED_VARIABLE;
    }

    int argIndex = 0;
    int argStart = start + 2;
    int depth = 0;

    /* No arguments */
    if (argStart == end) {
        if (func->paramCount == 0)
            return SUCCESS_CODE;

        printf("  Semantic error: wrong number of arguments for function '%s'\n", func->name);
        addErrorCode(lineNumber, ERROR_SEMANTIC_INVALID_IDENTIFIER_USE);
        return ERROR_SEMANTIC_INVALID_IDENTIFIER_USE;
    }

    for (int i = argStart; i <= end - 1; i++) {
        if (tokens[i].type == TOKEN_LPAREN)
            depth++;

        else if (tokens[i].type == TOKEN_RPAREN)
            depth--;

        if ((tokens[i].type == TOKEN_COMMA && depth == 0) || i == end - 1) {
            int argEnd = (tokens[i].type == TOKEN_COMMA) ? i - 1 : i;

            if (argIndex >= func->paramCount) {
                printf("  Semantic error: too many arguments for function '%s'\n", func->name);
                addErrorCode(lineNumber, ERROR_SEMANTIC_INVALID_IDENTIFIER_USE);
                return ERROR_SEMANTIC_INVALID_IDENTIFIER_USE;
            }

            int nestedResult = semanticCheckExpression(tokens, argStart, argEnd, lineNumber);

            if (nestedResult != SUCCESS_CODE)
                return nestedResult;

            char* argType = inferSemanticExpressionType(tokens, argStart, argEnd);

            if (strcmp(argType, func->paramTypes[argIndex]) != 0) {
                printf(
                    "  Semantic error: argument %d in function '%s' should be %s but got %s\n",
                    argIndex + 1,
                    func->name,
                    func->paramTypes[argIndex],
                    argType
                );

                addErrorCode(lineNumber, ERROR_SEMANTIC_INVALID_IDENTIFIER_USE);
                return ERROR_SEMANTIC_INVALID_IDENTIFIER_USE;
            }

            argIndex++;
            argStart = i + 1;
        }
    }

    if (argIndex != func->paramCount) {
        printf("  Semantic error: wrong number of arguments for function '%s'\n", func->name);
        addErrorCode(lineNumber, ERROR_SEMANTIC_INVALID_IDENTIFIER_USE);
        return ERROR_SEMANTIC_INVALID_IDENTIFIER_USE;
    }

    return SUCCESS_CODE;
}


int semanticCheckExpression(Token tokens[], int start, int end, int lineNumber)
{
    for (int i = start; i <= end; i++) {
        if (tokens[i].type == TOKEN_IDENTIFIER &&
            i + 1 <= end &&
            tokens[i + 1].type == TOKEN_LPAREN) {

            int depth = 0;
            int callEnd = -1;

            for (int j = i + 1; j <= end; j++) {
                if (tokens[j].type == TOKEN_LPAREN)
                    depth++;
                else if (tokens[j].type == TOKEN_RPAREN)
                    depth--;

                if (depth == 0) {
                    callEnd = j;
                    break;
                }
            }

            if (callEnd != -1) {
                int result = semanticCheckFunctionCallTokens(tokens, i, callEnd, lineNumber);

                if (result != SUCCESS_CODE)
                    return result;

                i = callEnd;
            }
        }
    }

    return SUCCESS_CODE;
}


/* ================================
   SEMANTIC CHECKS FOR STATEMENTS
   ================================ */

int semanticAssignment(TokenLine* line)
{
    SymbolNode* existing = findSymbol(line->tokens[0].value);

    if (existing != NULL && strcmp(existing->role, "constant") == 0) {
        printf("  Semantic error: cannot reassign constant '%s'\n", line->tokens[0].value);
        addErrorCode(line->lineNumber, ERROR_SEMANTIC_INVALID_IDENTIFIER_USE);
        return ERROR_SEMANTIC_INVALID_IDENTIFIER_USE;
    }

    /* += or -= */
    if (line->tokens[1].type == TOKEN_OPERATOR_PLUS_ASSIGN ||
        line->tokens[1].type == TOKEN_OPERATOR_MINUS_ASSIGN) {

        SymbolNode* leftSymbol = findSymbol(line->tokens[0].value);

        if (leftSymbol == NULL) {
            printf("  Semantic error: variable '%s' used before definition\n", line->tokens[0].value);
            addErrorCode(line->lineNumber, ERROR_SEMANTIC_UNDEFINED_VARIABLE);
            return ERROR_SEMANTIC_UNDEFINED_VARIABLE;
        }

        int result = checkIdentifiersDefined(line->tokens, 2, line->count - 1, line->lineNumber);
        if (result != SUCCESS_CODE)
            return result;

        result = checkIdentifiersInitialized(line->tokens, 2, line->count - 1, line->lineNumber, line->indentLevel);
        if (result != SUCCESS_CODE)
            return result;

        char* rightType = inferSemanticExpressionType(line->tokens, 2, line->count - 1);

        if (strcmp(leftSymbol->type, rightType) != 0) {
            printf(
                "  Semantic error: type mismatch in assignment: %s %s %s\n",
                leftSymbol->type,
                line->tokens[1].value,
                rightType
            );

            addErrorCode(line->lineNumber, ERROR_SEMANTIC_INVALID_IDENTIFIER_USE);
            return ERROR_SEMANTIC_INVALID_IDENTIFIER_USE;
        }

        markInitialized(line->tokens[0].value, getCurrentScope(), line->indentLevel);

        return SUCCESS_CODE;
    }

    /* Normal assignment */
    int result = checkIdentifiersDefined(line->tokens, 2, line->count - 1, line->lineNumber);
    if (result != SUCCESS_CODE)
        return result;

    result = semanticCheckExpression(line->tokens, 2, line->count - 1, line->lineNumber);
    if (result != SUCCESS_CODE)
        return result;

    result = checkIdentifiersInitialized(line->tokens, 2, line->count - 1, line->lineNumber, line->indentLevel);
    if (result != SUCCESS_CODE)
        return result;

    result = checkInvalidArithmeticTypes(line->tokens, 2, line->count - 1, line->lineNumber);
    if (result != SUCCESS_CODE)
        return result;

    if (existing != NULL) {
        char* newType = inferSemanticExpressionType(line->tokens, 2, line->count - 1);
        updateSymbolType(line->tokens[0].value, newType);

        markInitialized(line->tokens[0].value, getCurrentScope(), line->indentLevel);

        return SUCCESS_CODE;
    }

    char* role = isConstantName(line->tokens[0].value) ? "constant" : "variable";

    addSymbol(
        line->tokens[0].value,
        inferSemanticExpressionType(line->tokens, 2, line->count - 1),
        role,
        line->lineNumber
    );

    markInitialized(line->tokens[0].value, getCurrentScope(), line->indentLevel);

    return SUCCESS_CODE;
}


int semanticCondition(TokenLine* line)
{
    unsigned short resultCode = checkIdentifiersDefined(line->tokens, 1, line->count - 2, line->lineNumber);
    if (resultCode != SUCCESS_CODE)
        return resultCode;

    resultCode = checkIdentifiersInitialized(line->tokens, 1, line->count - 2, line->lineNumber, line->indentLevel);
    if (resultCode != SUCCESS_CODE)
        return resultCode;

    for (int i = 1; i < line->count - 1; i++) {
        if (line->tokens[i].type == TOKEN_OPERATOR_LT ||
            line->tokens[i].type == TOKEN_OPERATOR_GT ||
            line->tokens[i].type == TOKEN_OPERATOR_LTE ||
            line->tokens[i].type == TOKEN_OPERATOR_GTE) {

            char* leftType = getArgumentType(line->tokens[i - 1]);
            char* rightType = getArgumentType(line->tokens[i + 1]);

            if (!isNumericType(leftType) || !isNumericType(rightType)) {
                printf(
                    "  Semantic error: cannot compare %s with %s using '%s'\n",
                    leftType,
                    rightType,
                    line->tokens[i].value
                );

                addErrorCode(line->lineNumber, ERROR_SEMANTIC_INVALID_IDENTIFIER_USE);
                return ERROR_SEMANTIC_INVALID_IDENTIFIER_USE;
            }
        }
    }

    return SUCCESS_CODE;
}


int semanticPrint(TokenLine* line)
{
    int result = checkIdentifiersDefined(line->tokens, 2, line->count - 2, line->lineNumber);
    if (result != SUCCESS_CODE)
        return result;

    result = checkIdentifiersInitialized(line->tokens, 2, line->count - 2, line->lineNumber, line->indentLevel);
    if (result != SUCCESS_CODE)
        return result;

    return SUCCESS_CODE;
}


int semanticFor(TokenLine* line)
{
    addSymbol(line->tokens[1].value, "int", "variable", line->lineNumber);

    markInitialized(line->tokens[1].value, getCurrentScope(), line->indentLevel);

    return SUCCESS_CODE;
}


int semanticFunctionDef(TokenLine* line)
{
    strcpy(currentFunctionName, line->tokens[1].value);

    SymbolNode* existingFunc = findSymbolInScope(line->tokens[1].value, "global");

    if (existingFunc != NULL && strcmp(existingFunc->role, "function") == 0) {
        printf("  Semantic error: function '%s' already defined\n", line->tokens[1].value);
        addErrorCode(line->lineNumber, ERROR_SEMANTIC_INVALID_IDENTIFIER_USE);
        return ERROR_SEMANTIC_INVALID_IDENTIFIER_USE;
    }

    addSymbolToScope(line->tokens[1].value, "int", "function", "global", line->lineNumber);
    setCurrentScope(line->tokens[1].value);

    FunctionInfo* funcInfo = addFunctionInfo(line->tokens[1].value);

    int paramStart = 3;
    int paramEnd = line->count - 3;

    for (int i = paramStart; i <= paramEnd; i++) {
        if (line->tokens[i].type == TOKEN_IDENTIFIER) {
            char* type = "int";

            if (i + 2 <= paramEnd &&
                line->tokens[i + 1].type == TOKEN_COLON) {

                type = pythonTypeToCType(line->tokens[i + 2].type);
            }

            addSymbol(line->tokens[i].value, type, "parameter", line->lineNumber);

            /* parameters are initialized by definition */
            markInitialized(line->tokens[i].value, getCurrentScope(), line->indentLevel);

            if (funcInfo != NULL && funcInfo->paramCount < MAX_PARAMS) {
                strcpy(funcInfo->paramTypes[funcInfo->paramCount], type);
                funcInfo->paramCount++;
            }
        }
    }

    return SUCCESS_CODE;
}


int semanticReturn(TokenLine* line)
{
    int result = checkIdentifiersDefined(line->tokens, 1, line->count - 1, line->lineNumber);
    if (result != SUCCESS_CODE)
        return result;

    result = checkIdentifiersInitialized(line->tokens, 1, line->count - 1, line->lineNumber, line->indentLevel);
    if (result != SUCCESS_CODE)
        return result;

    result = checkInvalidArithmeticTypes(line->tokens, 1, line->count - 1, line->lineNumber);
    if (result != SUCCESS_CODE)
        return result;

    if (strlen(currentFunctionName) > 0) {
        char* returnType = "void";

        if (line->count > 1)
            returnType = inferSemanticExpressionType(line->tokens, 1, line->count - 1);

        addSymbolToScope(currentFunctionName, returnType, "function", "global", line->lineNumber);
        updateFunctionReturnType(currentFunctionName, returnType);
    }

    return SUCCESS_CODE;
}


int semanticFunctionCall(TokenLine* line)
{
    return semanticCheckFunctionCallTokens(
        line->tokens,
        ZERO,
        line->count - ONE,
        line->lineNumber
    );
}

unsigned short IsGlobal() {
    return strcmp(getCurrentScope(), "global");
}

int semanticCheck(TokenLine* line)
{
    if (line->count == ZERO)
        return SUCCESS_CODE;

    if (line->indentLevel == ZERO && IsGlobal() != ZERO) {
        setCurrentScope("global");
        currentFunctionName[ZERO] = EOD;
    }

    switch (line->tokens[ZERO].type)
    {
    case TOKEN_IDENTIFIER:
        if (line->count >= TWO && line->tokens[ONE].type == TOKEN_LPAREN)
            return semanticFunctionCall(line);
        return semanticAssignment(line);

    case TOKEN_KEYWORD_IF:
    case TOKEN_KEYWORD_ELIF:
    case TOKEN_KEYWORD_WHILE:
        return semanticCondition(line);

    case TOKEN_KEYWORD_FOR:
        return semanticFor(line);

    case TOKEN_KEYWORD_PRINT:
        return semanticPrint(line);

    case TOKEN_KEYWORD_DEF:
        return semanticFunctionDef(line);

    case TOKEN_KEYWORD_RETURN:
        return semanticReturn(line);

    default:
        return SUCCESS_CODE;
    }
}