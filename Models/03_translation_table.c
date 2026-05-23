
#include "compiler_common.h"

typedef struct TranslationNode {
    char key[MAX_WORD];
    char value[MAX_WORD];
    struct TranslationNode* next;
} TranslationNode;

TranslationNode* translationTable[TRANSLATION_TABLE_SIZE];

/*
 * Maps a translation key to a translation-table bucket.
 * Time complexity: O(n), where n is key length.
 */
int hashTranslationKey(char* key)
{
    int hash = 0;

    for (int i = 0; key[i] != '\0'; i++)
        hash = (hash * 31 + key[i]) % TRANSLATION_TABLE_SIZE;

    return hash;
}

/*
 * Adds a Python-to-C translation mapping.
 * Time complexity: O(n), where n is key length.
 */
void addTranslation(char* key, char* value)
{
    int index = hashTranslationKey(key);

    TranslationNode* newNode = (TranslationNode*)malloc(sizeof(TranslationNode));
    if (newNode == NULL)
        return;

    strcpy(newNode->key, key);
    strcpy(newNode->value, value);
    newNode->next = translationTable[index];
    translationTable[index] = newNode;
}


char* findTranslation(char* key)
{
    int index = hashTranslationKey(key);
    TranslationNode* current = translationTable[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0)
            return current->value;

        current = current->next;
    }

    return NULL;
}

/*
 * Initializes all translation mappings used by the translator.
 * Time complexity: O(T), where T is the number of inserted translations.
 */
void initTranslationTable()
{
    for (int i = 0; i < TRANSLATION_TABLE_SIZE; i++)
        translationTable[i] = NULL;

    addTranslation("if", "if");
    addTranslation("elif", "else if");
    addTranslation("while", "while");
    addTranslation("else", "else");
    addTranslation("print", "printf");
    addTranslation("def", "void");
    addTranslation("return", "return");
    addTranslation("True", "1");
    addTranslation("False", "0");
    addTranslation("and", "&&");
    addTranslation("or", "||");
    addTranslation("not", "!");
    addTranslation("=", "=");
    addTranslation("+=", "+=");
    addTranslation("-=", "-=");
    addTranslation("==", "==");
    addTranslation("!=", "!=");
    addTranslation(">=", ">=");
    addTranslation("<=", "<=");
    addTranslation(">", ">");
    addTranslation("<", "<");
    addTranslation("+", "+");
    addTranslation("-", "-");
    addTranslation("*", "*");
    addTranslation("/", "/");
    addTranslation("%", "%");
    addTranslation(":", "{");
    addTranslation("(", "(");
    addTranslation(")", ")");


    addTranslation("TOKEN_KEYWORD_IF", "if");
    addTranslation("TOKEN_KEYWORD_ELIF", "else if");
    addTranslation("TOKEN_KEYWORD_ELSE", "else");
    addTranslation("TOKEN_KEYWORD_WHILE", "while");
    addTranslation("TOKEN_KEYWORD_PRINT", "printf");
    addTranslation("TOKEN_KEYWORD_DEF", "void");
    addTranslation("TOKEN_KEYWORD_RETURN", "return");

    addTranslation("TOKEN_KEYWORD_TRUE", "1");
    addTranslation("TOKEN_KEYWORD_FALSE", "0");
    addTranslation("TOKEN_KEYWORD_AND", "&&");
    addTranslation("TOKEN_KEYWORD_OR", "||");
    addTranslation("TOKEN_KEYWORD_NOT", "!");

    addTranslation("TOKEN_OPERATOR_ASSIGN", "=");
    addTranslation("TOKEN_OPERATOR_PLUS_ASSIGN", "+=");
    addTranslation("TOKEN_OPERATOR_MINUS_ASSIGN", "-=");
    addTranslation("TOKEN_OPERATOR_EQUAL", "==");
    addTranslation("TOKEN_OPERATOR_NOT_EQUAL", "!=");
    addTranslation("TOKEN_OPERATOR_GTE", ">=");
    addTranslation("TOKEN_OPERATOR_LTE", "<=");
    addTranslation("TOKEN_OPERATOR_GT", ">");
    addTranslation("TOKEN_OPERATOR_LT", "<");
    addTranslation("TOKEN_OPERATOR_PLUS", "+");
    addTranslation("TOKEN_OPERATOR_MINUS", "-");
    addTranslation("TOKEN_OPERATOR_MULTIPLY", "*");
    addTranslation("TOKEN_OPERATOR_DIVIDE", "/");
    addTranslation("TOKEN_OPERATOR_MODULO", "%");

    addTranslation("TOKEN_LPAREN", "(");
    addTranslation("TOKEN_RPAREN", ")");
    addTranslation("TOKEN_COLON", "{");

    addTranslation("ctype:int", "int");
    addTranslation("ctype:float", "float");
    addTranslation("ctype:char*", "char*");

    addTranslation("printf:int", "%d");
    addTranslation("printf:float", "%f");
    addTranslation("printf:char*", "%s");
}

void freeTranslationTable()
{
    for (int i = 0; i < TRANSLATION_TABLE_SIZE; i++) {
        TranslationNode* current = translationTable[i];

        while (current != NULL) {
            TranslationNode* temp = current;
            current = current->next;
            free(temp);
        }

        translationTable[i] = NULL;
    }
}

void printTranslationTable()
{
    printf("\n\n--- Translation Hash Table Results ---\n");

    for (int i = 0; i < TRANSLATION_TABLE_SIZE; i++) {
        TranslationNode* current = translationTable[i];

        while (current != NULL) {
            printf("%-10s -> %s\n", current->key, current->value);
            current = current->next;
        }
    }
}