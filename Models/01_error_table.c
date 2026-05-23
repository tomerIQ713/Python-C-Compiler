
#include "compiler_common.h"

typedef struct ErrorNode {
    int lineNumber;
    int errorCode;
    struct ErrorNode* next;
} ErrorNode;

ErrorNode* errorTable[ERROR_TABLE_SIZE];

/*
 * Maps a source line number to an index in the error hash table.
 * O(1).
 */
int hashLine(int lineNumber)
{
    return lineNumber % ERROR_TABLE_SIZE;
}

/*
 * Initializes the error hash table before compilation starts.
 * O(E), where E is ERROR_TABLE_SIZE.
 */
void initErrorTable()
{
    for (int i = 0; i < ERROR_TABLE_SIZE; i++)
        errorTable[i] = NULL;
}

/*
 * Stores the error code found for a specific source line.
 * Time complexity: O(k), where k is the number of nodes
 */
void addErrorCode(int lineNumber, int errorCode)
{
    int index = hashLine(lineNumber);
    ErrorNode* current = errorTable[index];

    while (current != NULL) {
        if (current->lineNumber == lineNumber) {
            if (current->errorCode == SUCCESS_CODE)
                current->errorCode = errorCode;
            return;
        }
        current = current->next;
    }

    ErrorNode* newNode = (ErrorNode*)malloc(sizeof(ErrorNode));
    if (newNode == NULL)
        return;

    newNode->lineNumber = lineNumber;
    newNode->errorCode = errorCode;
    newNode->next = errorTable[index];

    errorTable[index] = newNode;
}

/*
 * Retrieves the error code for a specific source line.
 * Time complexity: O(k), where k is the number of nodes
 */
int getErrorCode(int lineNumber)
{
    int index = hashLine(lineNumber);
    ErrorNode* current = errorTable[index];

    while (current != NULL) {
        if (current->lineNumber == lineNumber)
            return current->errorCode;
        current = current->next;
    }

    return SUCCESS_CODE;
}

/*
 * Releases all dynamically allocated error-table nodes.
 * Time complexity: O(n + E), where n is stored errors and E is ERROR_TABLE_SIZE.
 */
void freeErrorTable()
{
    for (int i = 0; i < ERROR_TABLE_SIZE; i++) {
        ErrorNode* current = errorTable[i];

        while (current != NULL) {
            ErrorNode* temp = current;
            current = current->next;
            free(temp);
        }

        errorTable[i] = NULL;
    }
}

/*
 * Purpose: Converts an error code into a readable error name.
 * What it does: Uses a switch statement to map numeric codes to strings.
 * Time complexity: O(1).
 */
char* errorCodeToString(int errorCode)
{
    switch (errorCode) {
    case SUCCESS_CODE:
        return "SUCCESS";
    case ERROR_UNKNOWN_TOKEN:
        return "ERROR_UNKNOWN_TOKEN";
    case ERROR_INVALID_OPERATOR:
        return "ERROR_INVALID_OPERATOR";
    case ERROR_SYNTAX_INVALID_STATEMENT:
        return "ERROR_SYNTAX_INVALID_STATEMENT";
    case ERROR_SYNTAX_EXPECTED_COLON:
        return "ERROR_SYNTAX_EXPECTED_COLON";
    case ERROR_SYNTAX_EXPECTED_EXPRESSION:
        return "ERROR_SYNTAX_EXPECTED_EXPRESSION";
    case ERROR_SYNTAX_EXPECTED_ASSIGNMENT:
        return "ERROR_SYNTAX_EXPECTED_ASSIGNMENT";
    case ERROR_SYNTAX_EXPECTED_INDENT:
        return "ERROR_SYNTAX_EXPECTED_INDENT";
    case ERROR_SYNTAX_UNEXPECTED_INDENT:
        return "ERROR_SYNTAX_UNEXPECTED_INDENT";
    case ERROR_SYNTAX_INCONSISTENT_INDENT:
        return "ERROR_SYNTAX_INCONSISTENT_INDENT";
    case ERROR_SYNTAX_EXPECTED_PARENTHESES:
        return "ERROR_SYNTAX_EXPECTED_PARENTHESES";
    case ERROR_SYNTAX_INVALID_FUNCTION:
        return "ERROR_SYNTAX_INVALID_FUNCTION";
    case ERROR_SEMANTIC_UNDEFINED_VARIABLE:
        return "ERROR_SEMANTIC_UNDEFINED_VARIABLE";
    case ERROR_SEMANTIC_INVALID_IDENTIFIER_USE:
        return "ERROR_SEMANTIC_INVALID_IDENTIFIER_USE";
    case ERROR_SEMANTIC_UNINITIALIZED_VARIABLE:
        return "ERROR_SEMANTIC_UNINITIALIZED_VARIABLE";
    default:
        return "ERROR_UNKNOWN";
    }
}

