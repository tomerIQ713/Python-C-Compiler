#include "compiler_common.h"

#define NFA_SYMBOL_LETTER 1
#define NFA_SYMBOL_DIGIT 2

Automaton automata[MAX_AUTOMATA];
int automataCount = 0;

GeneratedAutomaton lexerAutomata[MAX_AUTOMATA];
int lexerAutomataCount = 0;

/*
 * checks whether a character belongs to the lexer letter category.
 * Time complexity: O(1).
 */
static int isLetterForLexer(char c)
{
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_';
}

/*
 * checks whether a character belongs to the lexer digit category.
 * Time complexity: O(1).
 */
static int isDigitForLexer(char c)
{
    return c >= '0' && c <= '9';
}

/*
 * prints the regex definitions used to generate the lexer automata.
 * O(1), because the printed text is fixed.
 */
void printAllLexerRegexes()
{
    printf("\n--- ALL LEXER REGEXES USED IN THE COMPILER ---\n");
    printf("Keywords: if | elif | else | while | print | str | int | float | for | in | range | def | return | True | False | and | or | not\n");
    printf("Operators: == | != | >= | <= | += | -= | = | > | < | + | - | * | / | %%\n");
    printf("Symbols: : | , | ( | )\n");
    printf("Values: @D@D* for number, @D@D*\\.@D@D* for float, @L(@L|@D)* for identifier\n");
    printf("Note: The lexer automata are generated using Thompson -> Subset -> Hopcroft.\n\n");
}

/*
 * creates and stores one generated automaton.
 * What it does: builds a minimized DFA from the regex, assigns the token type, and stores it in the generated lexer automata table.
 * O(R), where R is the full Regex -> NFA -> DFA -> minimized DFA pipeline cost.
 */
void addGeneratedAutomaton(char* name, char* regex, TokenType tokenType)
{
    if (lexerAutomataCount >= MAX_AUTOMATA)
        return;

    strcpy(lexerAutomata[lexerAutomataCount].name, name);
    lexerAutomata[lexerAutomataCount].dfa = buildAutomatonFromRegex(regex);
    lexerAutomata[lexerAutomataCount].tokenType = tokenType;

    if (lexerAutomata[lexerAutomataCount].dfa.stateCount > 0)
        lexerAutomataCount++;
}

/*
 * runs a generated DFA on one input word.
 * O(n), where n is the word length.
 */
int runGeneratedAutomaton(GeneratedAutomaton* a, char* word)
{
    int currentState = a->dfa.startState;

    for (int i = 0; word[i] != '\0'; i++) {
        unsigned char c = (unsigned char)word[i];
        int nextState = -1;

        if (c < MAX_SYMBOLS)
            nextState = a->dfa.transitions[currentState][c];

        if (nextState == -1 && isLetterForLexer((char)c))
            nextState = a->dfa.transitions[currentState][NFA_SYMBOL_LETTER];

        if (nextState == -1 && isDigitForLexer((char)c))
            nextState = a->dfa.transitions[currentState][NFA_SYMBOL_DIGIT];

        if (nextState == -1)
            return 0;

        currentState = nextState;
    }

    return a->dfa.acceptStates[currentState];
}

/*
 * initializes all lexer automata used by the compiler.
 * O(K * R), where K is the number of token regexes and R is the average automaton-generation cost.
 */
void initAutomata()
{
    automataCount = ZERO;
    lexerAutomataCount = ZERO;

    printf("Initializing generated lexical automata...\n");

    printAllLexerRegexes();

    addGeneratedAutomaton("Keyword_if", "if", TOKEN_KEYWORD_IF);
    addGeneratedAutomaton("Keyword_elif", "elif", TOKEN_KEYWORD_ELIF);
    addGeneratedAutomaton("Keyword_else", "else", TOKEN_KEYWORD_ELSE);
    addGeneratedAutomaton("Keyword_while", "while", TOKEN_KEYWORD_WHILE);
    addGeneratedAutomaton("Keyword_print", "print", TOKEN_KEYWORD_PRINT);

    addGeneratedAutomaton("Type_str", "str", TOKEN_KEYWORD_STR);
    addGeneratedAutomaton("Type_int", "int", TOKEN_KEYWORD_INT);
    addGeneratedAutomaton("Type_float", "float", TOKEN_KEYWORD_FLOAT);

    addGeneratedAutomaton("Keyword_for", "for", TOKEN_KEYWORD_FOR);
    addGeneratedAutomaton("Keyword_in", "in", TOKEN_KEYWORD_IN);
    addGeneratedAutomaton("Keyword_range", "range", TOKEN_KEYWORD_RANGE);

    addGeneratedAutomaton("Keyword_def", "def", TOKEN_KEYWORD_DEF);
    addGeneratedAutomaton("Keyword_return", "return", TOKEN_KEYWORD_RETURN);

    addGeneratedAutomaton("Boolean_true", "True", TOKEN_KEYWORD_TRUE);
    addGeneratedAutomaton("Boolean_false", "False", TOKEN_KEYWORD_FALSE);

    addGeneratedAutomaton("Logical_and", "and", TOKEN_KEYWORD_AND);
    addGeneratedAutomaton("Logical_or", "or", TOKEN_KEYWORD_OR);
    addGeneratedAutomaton("Logical_not", "not", TOKEN_KEYWORD_NOT);

    addGeneratedAutomaton("Operator_equal", "==", TOKEN_OPERATOR_EQUAL);
    addGeneratedAutomaton("Operator_not_equal", "!=", TOKEN_OPERATOR_NOT_EQUAL);
    addGeneratedAutomaton("Operator_greater_equal", ">=", TOKEN_OPERATOR_GTE);
    addGeneratedAutomaton("Operator_less_equal", "<=", TOKEN_OPERATOR_LTE);
    addGeneratedAutomaton("Operator_plus_assign", "+=", TOKEN_OPERATOR_PLUS_ASSIGN);
    addGeneratedAutomaton("Operator_minus_assign", "-=", TOKEN_OPERATOR_MINUS_ASSIGN);

    addGeneratedAutomaton("Operator_assign", "=", TOKEN_OPERATOR_ASSIGN);
    addGeneratedAutomaton("Operator_greater", ">", TOKEN_OPERATOR_GT);
    addGeneratedAutomaton("Operator_less", "<", TOKEN_OPERATOR_LT);
    addGeneratedAutomaton("Operator_plus", "+", TOKEN_OPERATOR_PLUS);
    addGeneratedAutomaton("Operator_minus", "-", TOKEN_OPERATOR_MINUS);
    addGeneratedAutomaton("Operator_multiply", "\\*", TOKEN_OPERATOR_MULTIPLY);
    addGeneratedAutomaton("Operator_divide", "/", TOKEN_OPERATOR_DIVIDE);
    addGeneratedAutomaton("Operator_modulo", "%", TOKEN_OPERATOR_MODULO);

    addGeneratedAutomaton("Comma", ",", TOKEN_COMMA);
    addGeneratedAutomaton("Colon", ":", TOKEN_COLON);
    addGeneratedAutomaton("Left_parenthesis", "\\(", TOKEN_LPAREN);
    addGeneratedAutomaton("Right_parenthesis", "\\)", TOKEN_RPAREN);

    addGeneratedAutomaton("Float_number", "@D@D*\\.@D@D*", TOKEN_FLOAT_NUMBER);
    addGeneratedAutomaton("Number", "@D@D*", TOKEN_NUMBER);
    addGeneratedAutomaton("Identifier", "@L(@L|@D)*", TOKEN_IDENTIFIER);

    printf("Generated lexical automata initialized successfully. Count: %d\n", lexerAutomataCount);
}














































































































































































































































































































































































































































































































































































































































































































































































































































