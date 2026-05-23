#ifndef COMPILER_COMMON_H
#define COMPILER_COMMON_H

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FUNCTIONS 200
#define MAX_PARAMS 20

#define MAX_TRANSITIONS 300
#define MAX_AUTOMATA 80
#define MAX_WORD 100
#define ERROR_TABLE_SIZE 101

#define MAX_STATES 100
#define MAX_REGEX 300
#define MAX_SYMBOLS 128
#define EPSILON 0

#define MAX_COMMENT 300

#define MAX_TOKENS_PER_LINE 50
#define SYMBOL_TABLE_SIZE 101
#define INDENT_STACK_SIZE 100
#define TRANSLATION_TABLE_SIZE 101
#define C_OUTPUT_SIZE 20000

#define FUNC_SIGNATURE_PLACEHOLDER_LEN 200

#define SUCCESS_CODE 0

#define ERROR_UNKNOWN_TOKEN 100
#define ERROR_INVALID_OPERATOR 102

#define ERROR_SYNTAX_INVALID_STATEMENT 200
#define ERROR_SYNTAX_EXPECTED_COLON 201
#define ERROR_SYNTAX_EXPECTED_EXPRESSION 202
#define ERROR_SYNTAX_EXPECTED_ASSIGNMENT 203
#define ERROR_SYNTAX_EXPECTED_INDENT 204
#define ERROR_SYNTAX_UNEXPECTED_INDENT 205
#define ERROR_SYNTAX_INCONSISTENT_INDENT 206
#define ERROR_SYNTAX_EXPECTED_PARENTHESES 207
#define ERROR_SYNTAX_INVALID_FUNCTION 208

#define ERROR_SEMANTIC_UNDEFINED_VARIABLE 300
#define ERROR_SEMANTIC_INVALID_IDENTIFIER_USE 301
#define ERROR_SEMANTIC_UNINITIALIZED_VARIABLE 302

#define GENERAL_ERROR 501

#define ZERO 0
#define ONE 1
#define TWO 2

typedef enum {
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELIF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_PRINT,
    TOKEN_KEYWORD_DEF,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_TRUE,
    TOKEN_KEYWORD_FALSE,
    TOKEN_KEYWORD_AND,
    TOKEN_KEYWORD_OR,
    TOKEN_KEYWORD_NOT,

    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_IN,
    TOKEN_KEYWORD_RANGE,

    TOKEN_KEYWORD_STR,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_FLOAT,

    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_FLOAT_NUMBER,
    TOKEN_STRING,

    TOKEN_OPERATOR_ASSIGN,
    TOKEN_OPERATOR_PLUS_ASSIGN,
    TOKEN_OPERATOR_MINUS_ASSIGN,
    TOKEN_OPERATOR_LT,
    TOKEN_OPERATOR_GT,
    TOKEN_OPERATOR_LTE,
    TOKEN_OPERATOR_GTE,
    TOKEN_OPERATOR_EQUAL,
    TOKEN_OPERATOR_NOT_EQUAL,
    TOKEN_OPERATOR_PLUS,
    TOKEN_OPERATOR_MINUS,
    TOKEN_OPERATOR_MULTIPLY,
    TOKEN_OPERATOR_DIVIDE,
    TOKEN_OPERATOR_MODULO,

    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,

    TOKEN_UNKNOWN
} TokenType;

typedef enum {
    SYMBOL_EXACT,
    SYMBOL_LETTER,
    SYMBOL_DIGIT
} SymbolType;

typedef struct {
    int fromState;
    SymbolType symbolType;
    char exactChar;
    int toState;
} Transition;

typedef struct {
    char name[30];
    int startState;
    int acceptState;
    int errorState;
    Transition transitions[MAX_TRANSITIONS];
    int transitionCount;
    TokenType tokenType;
} Automaton;

typedef struct {
    TokenType type;
    char value[MAX_WORD];
} Token;

typedef struct {
    Token tokens[MAX_TOKENS_PER_LINE];
    int count;
    int lineNumber;
    int indentLevel;

    int hasComment;
    char comment[MAX_COMMENT];
} TokenLine;

typedef struct {
    int fromState;
    TokenType tokenType;
    int toState;
    int acceptAny;
} TokenTransition;

typedef struct {
    int startState;
    int acceptState;
    TokenTransition transitions[120];
    int transitionCount;
} TokenAutomaton;

typedef struct {
    int indentStack[INDENT_STACK_SIZE];
    int top;
    int waitingForIndentedBlock;
    int blockStarterLine;
} ParserContext;

extern Automaton automata[MAX_AUTOMATA];
extern int automataCount;

typedef struct SymbolNode {
    char name[MAX_WORD];
    char type[20];
    char role[20];
    char scope[MAX_WORD];
    int lineDefined;
    struct SymbolNode* next;
} SymbolNode;

typedef struct {
    int indentStack[INDENT_STACK_SIZE];
    int top;
    int waitingForIndentedBlock;
    char cOutput[C_OUTPUT_SIZE];
    char globalOutput[C_OUTPUT_SIZE];
    int insideFunction;

    char pendingFuncName[MAX_WORD];
    char pendingFuncParams[MAX_WORD * 10];
    char pendingFuncComment[MAX_COMMENT];
    int pendingFuncOutputOffset;
} TranslationContext;

typedef struct {
    int from;
    char symbol;
    int to;
} NFATransition;

typedef struct {
    int startState;
    int acceptState;
    int stateCount;
    NFATransition transitions[MAX_STATES * 4];
    int transitionCount;
} NFA;

typedef struct {
    int stateCount;
    int startState;
    int acceptStates[MAX_STATES];
    int acceptCount;
    int transitions[MAX_STATES][MAX_SYMBOLS];
} DFA;

typedef struct {
    char name[30];
    DFA dfa;
    TokenType tokenType;
} GeneratedAutomaton;

extern GeneratedAutomaton lexerAutomata[MAX_AUTOMATA];
extern int lexerAutomataCount;

/* Automata */
int runAutomaton(Automaton* a, char* word);
int runGeneratedAutomaton(GeneratedAutomaton* a, char* word);
void initAutomata();
void addTokenTransition(TokenAutomaton* a, int from, TokenType type, int to);
void addTokenTransitionAny(TokenAutomaton* a, int from, int to);
int runTokenAutomaton(TokenAutomaton* a, Token tokens[], int count);
TokenAutomaton buildFunctionDefAutomaton();
TokenAutomaton buildFunctionCallAutomaton();
int parseFunctionCall(TokenLine* line);

/* Thompson / Subset / Hopcroft */
int isRegexCharacter(char c);
NFA thompsonConstruction(char* regex);
DFA subsetConstruction(NFA nfa);
DFA hopcroftMinimization(DFA dfa);
DFA buildAutomatonFromRegex(char* regex);

/* Error table */
void initErrorTable();
void addErrorCode(int lineNumber, int errorCode);
int getErrorCode(int lineNumber);
void freeErrorTable();
char* errorCodeToString(int errorCode);

/* Symbol table */
void initSymbolTable();
SymbolNode* findSymbol(char* name);
SymbolNode* findSymbolInScope(char* name, char* scope);
void addSymbol(char* name, char* type, char* role, int lineDefined);
void addSymbolToScope(char* name, char* type, char* role, char* scope, int lineDefined);
void updateSymbolType(char* name, char* newType);
void setCurrentScope(char* scope);
char* getCurrentScope();
void freeSymbolTable();
void printSymbolTable();

/* Translation table */
void initTranslationTable();
void freeTranslationTable();
void printTranslationTable();
void addTranslation(char* key, char* value);
char* findTranslation(char* key);

/* Parser */
void initParserContext(ParserContext* context);
int parsingCheck(TokenLine* line, ParserContext* context);

/* Semantic */
int semanticCheck(TokenLine* line);

/* Lexer */
char* GetNextLine(char* text, int* index);
TokenLine processLineToTokens(char* line, int lineNumber);
void singleQuotedStringToCString(char* source, char* destination);

/* Translator */
void initTranslationContext(TranslationContext* context);
void translateTokenLineToC(TokenLine* line, TranslationContext* context);
void closeAllTranslationBlocks(TranslationContext* context);
char* getCOutput(TranslationContext* context);
void translateExpression(Token tokens[], int start, int end, char* buffer);

/* Token translation helpers */
char* findTokenTranslation(Token token);
char* findCTypeTranslation(char* type);
char* findPrintfTranslation(char* type);
char* inferCTypeFromExpression(TokenLine* line, int start, int end);

/* File reader */
char* readFile(char* fileName);

/* Model results */
void printErrorTable(int totalLines);

/* View */
char* loadInputFile(char* fileName);
void viewShowFileError(char* fileName);
void viewShowMemoryError();
void ShowInfo(int totalLines);
void SaveOutput(char* fileName, char* globalOutput, char* cOutput);

/* Controller */
void InitCompiler(ParserContext* parserContext, TranslationContext* translationContext);
int ProcessTokenLine(TokenLine* tokenLine, ParserContext* parserContext);
void FreeSystem();

#endif