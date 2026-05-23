#include "compiler_common.h"

void patchFunctionSignature(TranslationContext* context, char* returnType);
void translateComment(TokenLine* line, TranslationContext* context);
void appendLineEnd(TranslationContext* context, TokenLine* line);

typedef struct {
    char pythonName[MAX_WORD];
    char cName[MAX_WORD];
    char type[20];
    char scope[MAX_WORD];
    int version;
} VariableAlias;

VariableAlias aliasTable[200];
int aliasCount = 0;
int functionSignaturePatched = 0;

/*
 * Finds a translated C variable alias in one scope.
 * O(A), where A is alias count.
 */
VariableAlias* findAliasInScope(char* pythonName, char* scope)
{
    for (int i = 0; i < aliasCount; i++) {
        if (strcmp(aliasTable[i].pythonName, pythonName) == 0 &&
            strcmp(aliasTable[i].scope, scope) == 0) {
            return &aliasTable[i];
        }
    }

    return NULL;
}

/*
 * Finds a variable alias in current or global scope.
 * O(A).
 */
VariableAlias* findAlias(char* pythonName)
{
    VariableAlias* local = findAliasInScope(pythonName, getCurrentScope());

    if (local != NULL)
        return local;

    return findAliasInScope(pythonName, "global");
}

char* getCName(char* pythonName)
{
    VariableAlias* alias = findAlias(pythonName);

    if (alias != NULL)
        return alias->cName;

    return pythonName;
}

/*
 * Creates or updates the C alias for a Python variable.
 * O(A).
 */
VariableAlias* createOrUpdateAlias(char* pythonName, char* type, int* shouldDeclare)
{
    char* scope = getCurrentScope();
    VariableAlias* alias = findAliasInScope(pythonName, scope);

    *shouldDeclare = 0;

    if (alias == NULL) {
        if (aliasCount >= 200)
            return NULL;

        strcpy(aliasTable[aliasCount].pythonName, pythonName);
        strcpy(aliasTable[aliasCount].cName, pythonName);
        strcpy(aliasTable[aliasCount].type, type);
        strcpy(aliasTable[aliasCount].scope, scope);
        aliasTable[aliasCount].version = 1;

        *shouldDeclare = 1;

        aliasCount++;
        return &aliasTable[aliasCount - 1];
    }

    if (strcmp(alias->type, type) != 0) {
        alias->version++;
        sprintf(alias->cName, "%s_%d", pythonName, alias->version);
        strcpy(alias->type, type);

        *shouldDeclare = 1;
    }

    return alias;
}


/*
 * Initializes C translation state.
 * O(1).
 */
void initTranslationContext(TranslationContext* context)
{
    context->top = 0;
    context->indentStack[0] = 0;
    context->waitingForIndentedBlock = 0;
    context->cOutput[0] = '\0';
    context->globalOutput[0] = '\0';
    context->insideFunction = 0;
    context->pendingFuncName[0] = '\0';
    context->pendingFuncParams[0] = '\0';
    context->pendingFuncOutputOffset = 0;
    context->pendingFuncComment[0] = '\0';

    aliasCount = 0;
    functionSignaturePatched = 0;
}

/*
 * Returns the current translation indentation level.
 * O(1).
 */
int currentTranslationIndent(TranslationContext* context)
{
    return context->indentStack[context->top];
}

void appendToCOutput(TranslationContext* context, char* text)
{
    if (context->insideFunction) {
        if (strlen(context->globalOutput) + strlen(text) < C_OUTPUT_SIZE - 1)
            strcat(context->globalOutput, text);
    }
    else {
        if (strlen(context->cOutput) + strlen(text) < C_OUTPUT_SIZE - 1)
            strcat(context->cOutput, text);
    }
}

/*
 * Appends text directly to the global output buffer.
 */
void appendToGlobalOutput(TranslationContext* context, char* text)
{
    if (strlen(context->globalOutput) + strlen(text) < C_OUTPUT_SIZE - 1)
        strcat(context->globalOutput, text);
}

/*
 * Appends indentation spaces to generated C output.
 */
void appendTabs(TranslationContext* context)
{
    if (!context->insideFunction)
        appendToCOutput(context, "    "); /* base indent inside main() */

    for (int i = 0; i < context->top; i++)
        appendToCOutput(context, "    ");
}

/*
 * Pushes a new translation indentation level.
 */
void pushTranslationIndent(TranslationContext* context, int indent)
{
    if (context->top < INDENT_STACK_SIZE - 1) {
        context->top++;
        context->indentStack[context->top] = indent;
    }
}

/*
 * Closes one translated C block.
 */
void popTranslationIndent(TranslationContext* context)
{
    if (context->top > 0) {
        context->top--;

        appendTabs(context);
        appendToCOutput(context, "}\n");

        if (context->top == 0 && context->insideFunction) {
            if (context->pendingFuncName[0] != '\0' && !functionSignaturePatched) {
                patchFunctionSignature(context, "void");
            }

            context->insideFunction = 0;
            context->pendingFuncName[0] = '\0';
            context->pendingFuncParams[0] = '\0';
            context->pendingFuncComment[0] = '\0';
            functionSignaturePatched = 0;
        }
    }
}

/*
 * Closes C blocks while the next Python line has lower indentation.
 * O(d), where d is number of closed blocks.
 */
void handleTranslationIndentation(TranslationContext* context, TokenLine* line)
{
    if (line->count == 0)
        return;

    while (line->indentLevel < currentTranslationIndent(context))
        popTranslationIndent(context);
}

/*
 * Replaces a function signature placeholder with the final typed C signature.
 * O(p * lookup), where p is parameter count.
 */
void patchFunctionSignature(TranslationContext* context, char* returnType)
{
    char signature[FUNC_SIGNATURE_PLACEHOLDER_LEN + 1];
    int pos = 0;

    int rtLen = (int)strlen(returnType);
    int nameLen = (int)strlen(context->pendingFuncName);

    memcpy(signature + pos, returnType, rtLen);
    pos += rtLen;

    signature[pos++] = ' ';

    memcpy(signature + pos, context->pendingFuncName, nameLen);
    pos += nameLen;

    signature[pos++] = '(';

    char paramsCopy[MAX_WORD * 10];
    int copyLen = (int)strlen(context->pendingFuncParams);
    memcpy(paramsCopy, context->pendingFuncParams, copyLen + 1);

    char* param = strtok(paramsCopy, ",");
    int first = 1;

    while (param != NULL) {
        SymbolNode* sym = findSymbolInScope(param, context->pendingFuncName);
        char* ctype = (sym != NULL) ? findCTypeTranslation(sym->type) : "int";

        int ctypeLen = (int)strlen(ctype);
        int paramLen = (int)strlen(param);

        if (!first) {
            signature[pos++] = ',';
            signature[pos++] = ' ';
        }

        memcpy(signature + pos, ctype, ctypeLen);
        pos += ctypeLen;

        signature[pos++] = ' ';

        memcpy(signature + pos, param, paramLen);
        pos += paramLen;

        first = 0;
        param = strtok(NULL, ",");
    }

    signature[pos++] = ')';
    signature[pos++] = '{';

    if (context->pendingFuncComment[0] != '\0') {
        char* commentPrefix = " // ";
        int prefixLen = (int)strlen(commentPrefix);
        int commentLen = (int)strlen(context->pendingFuncComment);

        if (pos + prefixLen + commentLen < FUNC_SIGNATURE_PLACEHOLDER_LEN) {
            memcpy(signature + pos, commentPrefix, prefixLen);
            pos += prefixLen;

            memcpy(signature + pos, context->pendingFuncComment, commentLen);
            pos += commentLen;
        }
    }

    while (pos < FUNC_SIGNATURE_PLACEHOLDER_LEN)
        signature[pos++] = ' ';
    signature[FUNC_SIGNATURE_PLACEHOLDER_LEN] = '\0';

    memcpy(
        context->globalOutput + context->pendingFuncOutputOffset,
        signature,
        FUNC_SIGNATURE_PLACEHOLDER_LEN
    );

    functionSignaturePatched = 1;
}

/*
 * Closes all open translated C blocks.
 * O(d).
 */
void closeAllTranslationBlocks(TranslationContext* context)
{
    while (context->top > 0)
        popTranslationIndent(context);
}


/*
 * Translates a token expression into C expression text.
 * O(k * lookup + output length).
 */
void translateExpression(Token tokens[], int start, int end, char* buffer)
{
    buffer[0] = '\0';

    for (int i = start; i <= end; i++) {
        char* translation;

        if (tokens[i].type == TOKEN_STRING) {
            char cString[MAX_WORD];
            singleQuotedStringToCString(tokens[i].value, cString);
            strcat(buffer, cString);
            continue;
        }

        translation = findTokenTranslation(tokens[i]);

        if (translation == NULL)
            translation = findTranslation(tokens[i].value);

        if (i > start &&
            tokens[i].type != TOKEN_RPAREN &&
            tokens[i].type != TOKEN_COMMA &&
            tokens[i - 1].type != TOKEN_LPAREN &&
            tokens[i].type != TOKEN_LPAREN) {
            strcat(buffer, " ");
        }

        if (translation != NULL) {
            strcat(buffer, translation);
        }
        else {
            if (tokens[i].type == TOKEN_IDENTIFIER)
                strcat(buffer, getCName(tokens[i].value));
            else
                strcat(buffer, tokens[i].value);
        }
    }
}


/*
 * Translates a standalone function call.
 * O(k * lookup).
 */
void translateFunctionCall(TokenLine* line, TranslationContext* context)
{
    char expression[500];

    translateExpression(line->tokens, 0, line->count - 1, expression);

    appendTabs(context);
    appendToCOutput(context, expression);
    appendToCOutput(context, ";\n");
}

/*
 * Translates an assignment statement.
 * O(k * lookup + A).
 */
void translateAssignment(TokenLine* line, TranslationContext* context)
{
    char expression[500];
    char* operatorTranslation;
    SymbolNode* symbol;

    char* currentType;
    VariableAlias* alias;
    int shouldDeclare = 0;

    translateExpression(line->tokens, 2, line->count - 1, expression);

    appendTabs(context);

    symbol = findSymbol(line->tokens[0].value);
    operatorTranslation = findTokenTranslation(line->tokens[1]);

    if (operatorTranslation == NULL)
        operatorTranslation = line->tokens[1].value;

    currentType = inferCTypeFromExpression(line, 2, line->count - 1);
    alias = createOrUpdateAlias(line->tokens[0].value, currentType, &shouldDeclare);

    if (alias == NULL)
        return;

    if (line->tokens[1].type == TOKEN_OPERATOR_PLUS_ASSIGN ||
        line->tokens[1].type == TOKEN_OPERATOR_MINUS_ASSIGN) {
        appendToCOutput(context, alias->cName);
        appendToCOutput(context, " ");
        appendToCOutput(context, operatorTranslation);
        appendToCOutput(context, " ");
        appendToCOutput(context, expression);
        appendToCOutput(context, ";");
        appendLineEnd(context, line);
        return;
    }

    if (symbol != NULL &&
        symbol->lineDefined == line->lineNumber &&
        strcmp(symbol->role, "parameter") != 0) {
        shouldDeclare = 1;
    }

    if (symbol != NULL && strcmp(symbol->role, "parameter") == 0 && shouldDeclare) {
        if (strcmp(alias->cName, line->tokens[0].value) == 0)
            shouldDeclare = 0;
    }

    if (shouldDeclare) {
        if (symbol != NULL && strcmp(symbol->role, "constant") == 0)
            appendToCOutput(context, "const ");

        appendToCOutput(context, findCTypeTranslation(alias->type));
        appendToCOutput(context, " ");
    }

    appendToCOutput(context, alias->cName);
    appendToCOutput(context, " ");
    appendToCOutput(context, operatorTranslation);
    appendToCOutput(context, " ");
    appendToCOutput(context, expression);
    appendToCOutput(context, ";");
    appendLineEnd(context, line);
}

/*
 * Translates an if/while condition line.
 * O(k * lookup).
 */
void translateConditionLine(TokenLine* line, TranslationContext* context)
{
    char condition[500];
    char* translatedKeyword;

    translateExpression(line->tokens, 1, line->count - 2, condition);

    translatedKeyword = findTokenTranslation(line->tokens[0]);

    if (translatedKeyword == NULL)
        translatedKeyword = line->tokens[0].value;

    appendTabs(context);
    appendToCOutput(context, translatedKeyword);
    appendToCOutput(context, "(");
    appendToCOutput(context, condition);
    appendToCOutput(context, "){\n");

    pushTranslationIndent(context, line->indentLevel + 4);
    context->waitingForIndentedBlock = 0;
}

/*
 * Translates Python elif into C else if.
 * O(k * lookup).
 */
void translateElif(TokenLine* line, TranslationContext* context)
{
    char condition[500];

    translateExpression(line->tokens, 1, line->count - 2, condition);

    appendTabs(context);
    appendToCOutput(context, "else if");
    appendToCOutput(context, "(");
    appendToCOutput(context, condition);
    appendToCOutput(context, "){\n");

    pushTranslationIndent(context, line->indentLevel + 4);
    context->waitingForIndentedBlock = 0;
}

/*
 * Translates Python else into a C else block.
 * O(1).
 */
void translateElse(TokenLine* line, TranslationContext* context)
{
    appendTabs(context);
    appendToCOutput(context, "else{\n");

    pushTranslationIndent(context, line->indentLevel + 4);
    context->waitingForIndentedBlock = 0;
}

/*
 * Translates a Python for-in-range loop into C.
 * O(k).
 */
void translateFor(TokenLine* line, TranslationContext* context)
{
    char var[MAX_WORD];
    char start[500];
    char stop[500];
    char step[500];

    int argStart;
    int argEnd;
    int argIndex;
    int depth;

    strcpy(var, line->tokens[1].value);

    strcpy(start, "0");
    strcpy(stop, "0");
    strcpy(step, "1");

    argStart = 5;
    argIndex = 0;
    depth = 0;

    for (int i = 5; i <= line->count - 2; i++) {
        if (line->tokens[i].type == TOKEN_LPAREN)
            depth++;

        else if (line->tokens[i].type == TOKEN_RPAREN)
            depth--;

        if ((line->tokens[i].type == TOKEN_COMMA && depth == 0) ||
            i == line->count - 2) {

            if (line->tokens[i].type == TOKEN_COMMA)
                argEnd = i - 1;
            else
                argEnd = i - 1;

            if (argIndex == 0) {
                translateExpression(line->tokens, argStart, argEnd, start);
            }
            else if (argIndex == 1) {
                translateExpression(line->tokens, argStart, argEnd, stop);
            }
            else if (argIndex == 2) {
                translateExpression(line->tokens, argStart, argEnd, step);
            }

            argIndex++;
            argStart = i + 1;
        }
    }

    if (argIndex == 1) {
        strcpy(stop, start);
        strcpy(start, "0");
        strcpy(step, "1");
    }
    else if (argIndex == 2) {
        strcpy(step, "1");
    }

    appendTabs(context);

    appendToCOutput(context, "for(int ");
    appendToCOutput(context, var);
    appendToCOutput(context, " = ");
    appendToCOutput(context, start);
    appendToCOutput(context, "; ");

    appendToCOutput(context, var);

    if (step[0] == '-')
        appendToCOutput(context, " > ");
    else
        appendToCOutput(context, " < ");

    appendToCOutput(context, stop);
    appendToCOutput(context, "; ");

    appendToCOutput(context, var);
    appendToCOutput(context, " += ");
    appendToCOutput(context, step);
    appendToCOutput(context, "){\n");

    pushTranslationIndent(context, line->indentLevel + 4);
    context->waitingForIndentedBlock = 0;
}

/*
 * Translates a Python function definition.
 * O(k).
 */
void translateFunctionDef(TokenLine* line, TranslationContext* context)
{
    context->insideFunction = 1;
    context->pendingFuncName[0] = '\0';
    context->pendingFuncParams[0] = '\0';
    functionSignaturePatched = 0;
    context->pendingFuncComment[0] = '\0';

    if (line->hasComment && strlen(line->comment) > 0) {
        strcpy(context->pendingFuncComment, line->comment);
    }

    int nameLen = (int)strlen(line->tokens[1].value);
    memcpy(context->pendingFuncName, line->tokens[1].value, nameLen + 1);

    int paramStart = 3;
    int paramEnd = line->count - 3;

    for (int i = paramStart; i <= paramEnd; i++) {
        if (line->tokens[i].type == TOKEN_IDENTIFIER) {
            if (context->pendingFuncParams[0] != '\0') {
                int pLen = (int)strlen(context->pendingFuncParams);
                context->pendingFuncParams[pLen] = ',';
                context->pendingFuncParams[pLen + 1] = '\0';
            }

            int tLen = (int)strlen(line->tokens[i].value);
            int pLen = (int)strlen(context->pendingFuncParams);
            memcpy(context->pendingFuncParams + pLen, line->tokens[i].value, tLen + 1);
        }
    }

    context->pendingFuncOutputOffset = (int)strlen(context->globalOutput);

    char placeholder[FUNC_SIGNATURE_PLACEHOLDER_LEN + 2];
    memset(placeholder, ' ', FUNC_SIGNATURE_PLACEHOLDER_LEN);
    placeholder[FUNC_SIGNATURE_PLACEHOLDER_LEN] = '\n';
    placeholder[FUNC_SIGNATURE_PLACEHOLDER_LEN + 1] = '\0';

    appendToGlobalOutput(context, placeholder);

    pushTranslationIndent(context, line->indentLevel + 4);
    context->waitingForIndentedBlock = 0;
}

/*
 * Translates a Python print call into printf.
 * O(k * lookup).
 */
void translatePrint(TokenLine* line, TranslationContext* context)
{
    char* printFunction = findTokenTranslation(line->tokens[0]);
    if (printFunction == NULL)
        printFunction = "printf";

    int start = 2;
    int end = line->count - 2;

    /* Check if expression contains + at top level (string concat case) */
    int hasPlus = 0;
    int depth = 0;
    for (int i = start; i <= end; i++) {
        if (line->tokens[i].type == TOKEN_LPAREN) depth++;
        else if (line->tokens[i].type == TOKEN_RPAREN) depth--;
        else if (line->tokens[i].type == TOKEN_OPERATOR_PLUS && depth == 0) {
            hasPlus = 1;
            break;
        }
    }

    if (hasPlus) {
        /* Split on top-level + and emit one printf per segment */
        int segStart = start;
        depth = 0;
        for (int i = start; i <= end + 1; i++) {
            int isSplit = (i == end + 1) ||
                (line->tokens[i].type == TOKEN_OPERATOR_PLUS && depth == 0);

            if (line->tokens[i].type == TOKEN_LPAREN) depth++;
            else if (line->tokens[i].type == TOKEN_RPAREN) depth--;

            if (isSplit) {
                int segEnd = (i == end + 1) ? end : i - 1;
                char segExpr[500];
                char segfmt[100];
                char* segType;

                translateExpression(line->tokens, segStart, segEnd, segExpr);
                segType = inferCTypeFromExpression(line, segStart, segEnd);
                char* fmt = findPrintfTranslation(segType);

                /* Last segment gets \n, others don't */
                if (i == end + 1)
                    sprintf(segfmt, "%s(\"%s\\n\", ", printFunction, fmt);
                else
                    sprintf(segfmt, "%s(\"%s\", ", printFunction, fmt);

                appendTabs(context);
                appendToCOutput(context, segfmt);
                appendToCOutput(context, segExpr);
                appendToCOutput(context, ");\n");

                segStart = i + 1;
            }
        }
        return;
    }

    /* Original single-expression path */
    char expression[500];
    char printfStart[100];
    translateExpression(line->tokens, start, end, expression);
    char* expressionType = inferCTypeFromExpression(line, start, end);
    char* format = findPrintfTranslation(expressionType);

    strcpy(printfStart, printFunction);
    strcat(printfStart, "(\"");
    strcat(printfStart, format);
    strcat(printfStart, "\\n\", ");

    appendTabs(context);
    appendToCOutput(context, printfStart);
    appendToCOutput(context, expression);
    appendToCOutput(context, ");\n");
}

/*
 * Translates a Python return statement.
 * O(k * lookup + p).
 */
void translateReturn(TokenLine* line, TranslationContext* context)
{
    appendTabs(context);
    appendToCOutput(context, "return");

    if (line->count > 1) {
        char expression[500];
        char* returnType = "int";

        translateExpression(line->tokens, 1, line->count - 1, expression);

        if (context->pendingFuncName[0] != '\0') {
            SymbolNode* funcSymbol = findSymbolInScope(context->pendingFuncName, "global");

            if (funcSymbol != NULL)
                returnType = funcSymbol->type;

            patchFunctionSignature(context, returnType);
        }

        appendToCOutput(context, " ");
        appendToCOutput(context, expression);
    }
    else {
        if (context->pendingFuncName[0] != '\0') {
            patchFunctionSignature(context, "void");
        }
    }

    appendToCOutput(context, ";");
    appendLineEnd(context, line);
}

/*
 * Translates one validated TokenLine into C code.
 * O(k * lookup + d) worst case.
 */
void translateTokenLineToC(TokenLine* line, TranslationContext* context)
{
    if (line->count == 0) {
        appendToCOutput(context, "\n");
        return;
    }

    handleTranslationIndentation(context, line);

    if (line->tokens[0].type == TOKEN_IDENTIFIER &&
        line->count >= 2 &&
        line->tokens[1].type == TOKEN_LPAREN) {
        translateFunctionCall(line, context);
        return;
    }

    switch (line->tokens[0].type)
    {
    case TOKEN_IDENTIFIER:
        translateAssignment(line, context);
        break;

    case TOKEN_KEYWORD_IF:
    case TOKEN_KEYWORD_WHILE:
        translateConditionLine(line, context);
        break;

    case TOKEN_KEYWORD_ELIF:
        translateElif(line, context);
        break;

    case TOKEN_KEYWORD_FOR:
        translateFor(line, context);
        break;

    case TOKEN_KEYWORD_DEF:
        translateFunctionDef(line, context);
        break;

    case TOKEN_KEYWORD_ELSE:
        translateElse(line, context);
        break;

    case TOKEN_KEYWORD_PRINT:
        translatePrint(line, context);
        break;

    case TOKEN_KEYWORD_RETURN:
        translateReturn(line, context);
        break;

    default:
        break;
    }
}

/*
 * Returns the generated main C output buffer.
 */
char* getCOutput(TranslationContext* context)
{
    return context->cOutput;
}

void translateComment(TokenLine* line, TranslationContext* context)
{
    if (!line->hasComment)
        return;

    if (strlen(line->comment) == 0)
        return;

    appendTabs(context);
    appendToCOutput(context, "// ");
    appendToCOutput(context, line->comment);
    appendToCOutput(context, "\n");
}

void appendLineEnd(TranslationContext* context, TokenLine* line)
{
    if (line->hasComment && strlen(line->comment) > 0) {
        appendToCOutput(context, " // ");
        appendToCOutput(context, line->comment);
    }

    appendToCOutput(context, "\n");
}