
#include "compiler_common.h"

SymbolNode* symbolTable[SYMBOL_TABLE_SIZE];
char currentScope[MAX_WORD] = "global";

/*
 * Maps an identifier name to a symbol-table bucket.
 * Time complexity: O(n), where n is the identifier length.
 */
int hashIdentifier(char* name)
{
    int hash = 0;

    for (int i = 0; name[i] != '\0'; i++)
        hash = (hash * 31 + name[i]) % SYMBOL_TABLE_SIZE;

    return hash;
}

/*
 * Updates the inferred type of an existing symbol.
 * O(n + k), where n is name length and k is O(findSymbol)
 */
void updateSymbolType(char* name, char* newType)
{
    SymbolNode* symbol = findSymbol(name);

    if (symbol != NULL)
        strcpy(symbol->type, newType);
}

/*
 * Changes the current semantic scope.
 * Copies the new scope name into the global currentScope buffer.
 * Time complexity: O(n), where n is scope length.
 */
void setCurrentScope(char* scope)
{
    strcpy(currentScope, scope);
}


char* getCurrentScope()
{
    return currentScope;
}

void initSymbolTable()
{
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++)
        symbolTable[i] = NULL;

    strcpy(currentScope, "global");
}

/*
 * Finds a symbol only inside a specific scope.
 * Time complexity: O(n + k), where n is name length and k is bucket size.
 */
SymbolNode* findSymbolInScope(char* name, char* scope)
{
    int index = hashIdentifier(name);
    SymbolNode* current = symbolTable[index];

    while (current != NULL) {
        if (strcmp(current->name, name) == 0 &&
            strcmp(current->scope, scope) == 0)
            return current;

        current = current->next;
    }

    return NULL;
}

SymbolNode* findSymbol(char* name)
{
    SymbolNode* local = findSymbolInScope(name, currentScope);

    if (local != NULL)
        return local;

    return findSymbolInScope(name, "global");
}

/*
 * Purpose: Adds or updates a symbol in a chosen scope.
 * Time complexity: O(n + k), where k is bucket size.
 */
void addSymbolToScope(char* name, char* type, char* role, char* scope, int lineDefined)
{
    SymbolNode* existing = findSymbolInScope(name, scope);

    if (existing != NULL) {
        strcpy(existing->type, type);
        strcpy(existing->role, role);
        strcpy(existing->scope, scope);
        existing->lineDefined = lineDefined;
        return;
    }

    int index = hashIdentifier(name);

    SymbolNode* newNode = (SymbolNode*)malloc(sizeof(SymbolNode));
    if (newNode == NULL)
        return;

    strcpy(newNode->name, name);
    strcpy(newNode->type, type);
    strcpy(newNode->role, role);
    strcpy(newNode->scope, scope);
    newNode->lineDefined = lineDefined;
    newNode->next = symbolTable[index];

    symbolTable[index] = newNode;
}


void addSymbol(char* name, char* type, char* role, int lineDefined)
{
    addSymbolToScope(name, type, role, currentScope, lineDefined);
}


void freeSymbolTable()
{
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        SymbolNode* current = symbolTable[i];

        while (current != NULL) {
            SymbolNode* temp = current;
            current = current->next;
            free(temp);
        }

        symbolTable[i] = NULL;
    }
}


void printSymbolTable()
{
    printf("\n\n--- Symbol Table Results ---\n");

    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        SymbolNode* current = symbolTable[i];

        while (current != NULL) {
            printf("Name: %-10s Type: %-10s Role: %-10s Scope: %-10s Defined at line: %d\n",
                current->name,
                current->type,
                current->role,
                current->scope,
                current->lineDefined);

            current = current->next;
        }
    }
}