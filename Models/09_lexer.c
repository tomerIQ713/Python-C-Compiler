#include "compiler_common.h"

/*
 * extracts the next source-code line from the input text.
 * O(L), where L is the line length.
 */
char* GetNextLine(char* text, int* index)
{
    int start = *index;
    int length = ZERO;

    while (text[*index] != '\0' && text[*index] != '\n') {
        (*index)++;
        length++;
    }

    if (length > ZERO && text[start + length - ONE] == '\r')
        length--;

    char* line = (char*)malloc(length + ONE);
    if (line == NULL)
        return NULL;

    for (unsigned int i = ZERO; i < length; i++)
        line[i] = text[start + i];

    line[length] = '\0';

    if (text[*index] == '\n')
        (*index)++;

    return line;
}

/*
 * checks whether a character should be treated as a separate token or operator.
 * O(1).
 */
int isSpecialSymbol(char c)
{
    return c == ':' || c == ',' || c == '(' || c == ')' ||
        c == '=' || c == '<' || c == '>' || c == '!' ||
        c == '-' || c == '+' || c == '*' || c == '/' || c == '%';
}

int isNonAscii(char c)
{
    return ((unsigned char)c > 127);
}

/*
 * counts the indentation level of a source line.
 * O(L), where L is the indentation prefix length.
 */
int countIndentation(char* line)
{
    int count = 0;

    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == ' ')
            count++;
        else if (line[i] == '\t')
            count += 4;
        else if (isNonAscii(line[i]))
            continue;
        else
            break;
    }

    return count;
}


Token makeToken(TokenType type, char* value)
{
    Token token;
    token.type = type;
    strcpy(token.value, value);
    return token;
}

/*
 * creates a string token object.
 * O(V), where V is the string length.
 */
Token makeStringToken(char* value)
{
    Token token;
    token.type = TOKEN_STRING;
    strcpy(token.value, value);
    return token;
}

/*
 * converts a Python-style quoted string into a C-style string literal.
 * O(n), where n is the string length.
 */
void singleQuotedStringToCString(char* source, char* destination)
{
    int outIndex = 0;
    char quote = source[0];

    destination[outIndex++] = '"';

    for (int i = 1; source[i] != '\0' && source[i] != quote; i++) {
        if (source[i] == '"' || source[i] == '\\')
            destination[outIndex++] = '\\';

        destination[outIndex++] = source[i];
    }

    destination[outIndex++] = '"';
    destination[outIndex] = '\0';
}

/*
 * adds a token to the current token line.
 * O(1).
 */
void addTokenToLine(TokenLine* tokenLine, Token token)
{
    if (tokenLine->count >= MAX_TOKENS_PER_LINE)
        return;

    tokenLine->tokens[tokenLine->count] = token;
    tokenLine->count++;
}

/*
 * classifies one word or symbol into a compiler token.
 * O(A * n), where A is the number of lexer automata and n is word length.
 */
Token processWord(char* word, int lineNumber)
{
    Token currentToken;
    unsigned short success = ZERO;

    for (unsigned int i = ZERO; i < lexerAutomataCount && !success; i++) {
        if (runGeneratedAutomaton(&lexerAutomata[i], word)) {
            printf("  %s -> %s\n", word, lexerAutomata[i].name);
            currentToken = makeToken(lexerAutomata[i].tokenType, word);
            success = ONE;
        }
    }

    if (!success) {
        printf("  %s -> ERROR_UNKNOWN_TOKEN\n", word);
        addErrorCode(lineNumber, ERROR_UNKNOWN_TOKEN);
        currentToken = makeToken(TOKEN_UNKNOWN, word);
    }

    return currentToken;
}

/*
 * performs lexical analysis on one source-code line.
 * O(L + A) in the worst case, where L is line length, A is automata count, and n is average token length.
 */
TokenLine processLineToTokens(char* line, int lineNumber)
{
    TokenLine tokenLine;
    tokenLine.count = 0;
    tokenLine.lineNumber = lineNumber;
    tokenLine.indentLevel = countIndentation(line);

    tokenLine.hasComment = 0;
    tokenLine.comment[0] = '\0';

    char word[MAX_WORD];
    int wordIndex = 0;

    addErrorCode(lineNumber, SUCCESS_CODE);

    printf("\nLine %d: %s\n", lineNumber, line);

    for (int i = 0; ; i++) {
        char c = line[i];

        if (isNonAscii(c)) {
            continue;
        }

        if (c == '#') {
            if (wordIndex > 0) {
                word[wordIndex] = '\0';
                addTokenToLine(&tokenLine, processWord(word, lineNumber));
                wordIndex = 0;
            }

            tokenLine.hasComment = 1;

            int commentStart = i + 1;

            while (line[commentStart] == ' ' || line[commentStart] == '\t')
                commentStart++;

            int commentIndex = 0;

            while (line[commentStart] != '\0' && commentIndex < MAX_COMMENT - 1) {
                tokenLine.comment[commentIndex] = line[commentStart];
                commentIndex++;
                commentStart++;
            }

            tokenLine.comment[commentIndex] = '\0';

            break;
        }

        if (c == '\'' || c == '"') {
            char quote = c;

            if (wordIndex > 0) {
                word[wordIndex] = '\0';
                addTokenToLine(&tokenLine, processWord(word, lineNumber));
                wordIndex = 0;
            }

            char stringLiteral[MAX_WORD];
            int stringIndex = 0;

            stringLiteral[stringIndex++] = quote;
            i++;

            while (line[i] != '\0' && line[i] != quote) {
                if (!isNonAscii(line[i]) && stringIndex < MAX_WORD - 2)
                    stringLiteral[stringIndex++] = line[i];

                i++;
            }

            if (line[i] == quote) {
                stringLiteral[stringIndex++] = quote;
                stringLiteral[stringIndex] = '\0';

                printf("  %s -> String\n", stringLiteral);
                addTokenToLine(&tokenLine, makeStringToken(stringLiteral));
            }
            else {
                stringLiteral[stringIndex] = '\0';

                printf("  %s -> ERROR_UNKNOWN_TOKEN\n", stringLiteral);
                addErrorCode(lineNumber, ERROR_UNKNOWN_TOKEN);
                addTokenToLine(&tokenLine, makeToken(TOKEN_UNKNOWN, stringLiteral));
                break;
            }
        }
        else if (c == ' ' || c == '\t' || c == '\0' || isSpecialSymbol(c)) {

            if (wordIndex > 0) {
                word[wordIndex] = '\0';
                addTokenToLine(&tokenLine, processWord(word, lineNumber));
                wordIndex = 0;
            }

            if (c == '=' || c == '!' || c == '<' || c == '>' || c == '+' || c == '-') {
                char op[4];
                op[0] = c;
                op[1] = '\0';
                op[2] = '\0';
                op[3] = '\0';
                
                if (c == '=' && line[i + 1] == '=' && line[i + 2] == '=') {
                    printf("  === -> ERROR_INVALID_OPERATOR\n");
                    addErrorCode(lineNumber, ERROR_INVALID_OPERATOR);
                    addTokenToLine(&tokenLine, makeToken(TOKEN_UNKNOWN, "==="));
                    i += 2;
                }
                else if (c == '!' && line[i + 1] != '=') {
                    printf("  ! -> ERROR_INVALID_OPERATOR\n");
                    addErrorCode(lineNumber, ERROR_INVALID_OPERATOR);
                    addTokenToLine(&tokenLine, makeToken(TOKEN_UNKNOWN, "!"));
                }
                else if ((c == '=' && line[i + 1] == '=') ||
                    (c == '!' && line[i + 1] == '=') ||
                    (c == '<' && line[i + 1] == '=') ||
                    (c == '>' && line[i + 1] == '=') ||
                    (c == '+' && line[i + 1] == '=') ||
                    (c == '-' && line[i + 1] == '=')) {

                    op[1] = '=';
                    op[2] = '\0';

                    addTokenToLine(&tokenLine, processWord(op, lineNumber));
                    i++;
                }
                else {
                    addTokenToLine(&tokenLine, processWord(op, lineNumber));
                }
            }
            else if (c == ':' || c == '(' || c == ')' || c == '*' || c == '/' || c == '%') {
                char symbol[2];
                symbol[0] = c;
                symbol[1] = '\0';

                addTokenToLine(&tokenLine, processWord(symbol, lineNumber));
            }
            else if (c == ',') {
                addTokenToLine(&tokenLine, processWord(",", lineNumber));
            }

            if (c == '\0')
                break;
        }
        else {
            if (wordIndex < MAX_WORD - 1) {
                word[wordIndex++] = c;
            }
            else {
                word[wordIndex] = '\0';

                printf("  %s -> ERROR_UNKNOWN_TOKEN\n", word);
                addErrorCode(lineNumber, ERROR_UNKNOWN_TOKEN);
                addTokenToLine(&tokenLine, makeToken(TOKEN_UNKNOWN, word));

                wordIndex = 0;
            }
        }
    }

    return tokenLine;
}
