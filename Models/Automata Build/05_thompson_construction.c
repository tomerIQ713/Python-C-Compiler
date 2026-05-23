#include "compiler_common.h"

#define NFA_SYMBOL_LETTER 1
#define NFA_SYMBOL_DIGIT 2

/*
 * What it does: compares the current transition count to the maximum allowed number of transitions.
 * O(1).
 */
static int canAddNFATransition(NFA* nfa)
{
    return nfa->transitionCount < MAX_STATES * 4;
}

/*
 * adds one transition to an NFA.
 * O(1).
 */
void addNFATransition(NFA* nfa, int from, char symbol, int to)
{
    if (!canAddNFATransition(nfa))
        return;

    nfa->transitions[nfa->transitionCount].from = from;
    nfa->transitions[nfa->transitionCount].symbol = symbol;
    nfa->transitions[nfa->transitionCount].to = to;
    nfa->transitionCount++;
}

NFA createEmptyNFA()
{
    NFA nfa;
    nfa.startState = 0;
    nfa.acceptState = 0;
    nfa.stateCount = 0;
    nfa.transitionCount = 0;
    return nfa;
}

/*
 * creates the basic Thompson NFA for one symbol.
 * O(1).
 */
NFA createBasicNFA(char symbol)
{
    NFA nfa = createEmptyNFA();

    nfa.startState = 0;
    nfa.acceptState = 1;
    nfa.stateCount = 2;

    addNFATransition(&nfa, 0, symbol, 1);

    return nfa;
}

/*
 * renumbers all states inside an NFA.
 * O(T), where T is the number of transitions in the NFA.
 */
void shiftNFAStates(NFA* nfa, int shift)
{
    nfa->startState += shift;
    nfa->acceptState += shift;

    for (int i = 0; i < nfa->transitionCount; i++) {
        nfa->transitions[i].from += shift;
        nfa->transitions[i].to += shift;
    }
}

/*
 * builds the Thompson concatenation of two NFAs.
 * O(Ta + Tb), where Ta and Tb are the numbers of transitions in the two NFAs.
 */
NFA concatNFA(NFA a, NFA b)
{
    shiftNFAStates(&b, a.stateCount);

    NFA result = createEmptyNFA();
    result.startState = a.startState;
    result.acceptState = b.acceptState;
    result.stateCount = a.stateCount + b.stateCount;

    for (int i = 0; i < a.transitionCount; i++)
        result.transitions[result.transitionCount++] = a.transitions[i];

    for (int i = 0; i < b.transitionCount; i++)
        result.transitions[result.transitionCount++] = b.transitions[i];

    addNFATransition(&result, a.acceptState, EPSILON, b.startState);

    return result;
}

/*
 * builds the Thompson union / OR construction between two NFAs.
 * O(Ta + Tb).
 */
NFA unionNFA(NFA a, NFA b)
{
    shiftNFAStates(&a, 1);
    shiftNFAStates(&b, a.stateCount + 1);

    NFA result = createEmptyNFA();

    result.startState = 0;
    result.acceptState = a.stateCount + b.stateCount + 1;
    result.stateCount = a.stateCount + b.stateCount + 2;

    for (int i = 0; i < a.transitionCount; i++)
        result.transitions[result.transitionCount++] = a.transitions[i];

    for (int i = 0; i < b.transitionCount; i++)
        result.transitions[result.transitionCount++] = b.transitions[i];

    addNFATransition(&result, result.startState, EPSILON, a.startState);
    addNFATransition(&result, result.startState, EPSILON, b.startState);
    addNFATransition(&result, a.acceptState, EPSILON, result.acceptState);
    addNFATransition(&result, b.acceptState, EPSILON, result.acceptState);

    return result;
}

/*
 * creates a new start and accept state, then adds epsilon transitions for skipping, entering, repeating, and exiting the repeated NFA.
 * O(T), where T is the number of transitions in the original NFA.
 */
NFA starNFA(NFA a)
{
    shiftNFAStates(&a, 1);

    NFA result = createEmptyNFA();

    result.startState = 0;
    result.acceptState = a.stateCount + 1;
    result.stateCount = a.stateCount + 2;

    for (int i = 0; i < a.transitionCount; i++)
        result.transitions[result.transitionCount++] = a.transitions[i];

    addNFATransition(&result, result.startState, EPSILON, a.startState);
    addNFATransition(&result, result.startState, EPSILON, result.acceptState);
    addNFATransition(&result, a.acceptState, EPSILON, a.startState);
    addNFATransition(&result, a.acceptState, EPSILON, result.acceptState);

    return result;
}

int isRegexCharacter(char c)
{
    return
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' ||
        c == '=' ||
        c == '<' ||
        c == '>' ||
        c == ':' ||
        c == ',' ||
        c == '+' ||
        c == '-' ||
        c == '/' ||
        c == '%' ||
        c == '!';
}

/*
 * returns the precedence level of a regex operator.
 * O(1).
 */
int precedence(char op)
{
    if (op == '|')
        return 1;

    if (op == '.')
        return 2;

    return 0;
}

/*
 * checks whether a regex character is escaped.
 * O(1).
 */
static int isEscapedRegexItem(char* regex, int index)
{
    return index > 0 && regex[index - 1] == '\\';
}

/*
 * checks whether the current character is part of an internal category token.
 * O(1).
 */
static int isCategoryRegexItem(char* regex, int index)
{
    return index > 0 && regex[index - 1] == '@' &&
        (regex[index] == 'L' || regex[index] == 'D');
}

/*
 * checks whether the current regex item can legally end an expression part.
 * O(1).
 */
static int canEndRegexItem(char* regex, int index)
{
    char c = regex[index];

    if (isEscapedRegexItem(regex, index))
        return 1;

    if (isCategoryRegexItem(regex, index))
        return 1;

    return isRegexCharacter(c) || c == ')' || c == '*';
}

/*
 * checks whether the next regex item can legally start an expression part.
 * O(1).
 */
static int canStartRegexItem(char* regex, int index)
{
    char c = regex[index];

    if (c == '\\')
        return regex[index + 1] != '\0';

    if (c == '@')
        return regex[index + 1] == 'L' || regex[index + 1] == 'D';

    return isRegexCharacter(c) || c == '(';
}

/*
 * detects implicit concatenation between two regex items.
 * O(1).
 */
int needsConcatAt(char* regex, int currentIndex)
{
    if (regex[currentIndex] == '\0' || regex[currentIndex + 1] == '\0')
        return 0;

    return canEndRegexItem(regex, currentIndex) &&
        canStartRegexItem(regex, currentIndex + 1);
}

/*
 * applies one regex operator to the NFA stack.
 * O(T), where T is the total number of transitions in the NFA fragments being combined.
 */
void applyOperator(NFA stack[], int* nfaTop, char op)
{
    if (op == '*') {
        if (*nfaTop < 0)
            return;

        NFA a = stack[(*nfaTop)--];
        stack[++(*nfaTop)] = starNFA(a);
        return;
    }

    if (*nfaTop < 1)
        return;

    NFA b = stack[(*nfaTop)--];
    NFA a = stack[(*nfaTop)--];

    if (op == '.')
        stack[++(*nfaTop)] = concatNFA(a, b);
    else if (op == '|')
        stack[++(*nfaTop)] = unionNFA(a, b);
}

/*
 * converts a supported regular expression into an NFA using Thompson Construction.
 * O(n * T) in this implementation, where n is regex length and T is the number of transitions copied during NFA combinations.
 */
NFA thompsonConstruction(char* regex)
{
    NFA* stack = (NFA*)malloc(sizeof(NFA) * MAX_REGEX);
    char* operators = (char*)malloc(sizeof(char) * MAX_REGEX);

    int nfaTop = -1;
    int opTop = -1;

    if (stack == NULL || operators == NULL) {
        if (stack != NULL)
            free(stack);

        if (operators != NULL)
            free(operators);

        return createBasicNFA(' ');
    }

    for (int i = 0; regex[i] != '\0'; i++) {
        char c = regex[i];

        if (c == '\\' && regex[i + 1] != '\0') {
            i++;
            stack[++nfaTop] = createBasicNFA(regex[i]);
        }
        else if (c == '@' && (regex[i + 1] == 'L' || regex[i + 1] == 'D')) {
            i++;

            if (regex[i] == 'L')
                stack[++nfaTop] = createBasicNFA((char)NFA_SYMBOL_LETTER);
            else
                stack[++nfaTop] = createBasicNFA((char)NFA_SYMBOL_DIGIT);
        }
        else if (isRegexCharacter(c)) {
            stack[++nfaTop] = createBasicNFA(c);
        }
        else if (c == '(') {
            operators[++opTop] = c;
        }
        else if (c == ')') {
            while (opTop >= 0 && operators[opTop] != '(') {
                applyOperator(stack, &nfaTop, operators[opTop]);
                opTop--;
            }

            if (opTop >= 0 && operators[opTop] == '(')
                opTop--;
        }
        else if (c == '*') {
            applyOperator(stack, &nfaTop, '*');
        }
        else if (c == '|') {
            while (opTop >= 0 &&
                operators[opTop] != '(' &&
                precedence(operators[opTop]) >= precedence(c)) {
                applyOperator(stack, &nfaTop, operators[opTop]);
                opTop--;
            }

            operators[++opTop] = c;
        }

        if (needsConcatAt(regex, i)) {
            while (opTop >= 0 &&
                operators[opTop] != '(' &&
                precedence(operators[opTop]) >= precedence('.')) {
                applyOperator(stack, &nfaTop, operators[opTop]);
                opTop--;
            }

            operators[++opTop] = '.';
        }
    }

    while (opTop >= 0) {
        applyOperator(stack, &nfaTop, operators[opTop]);
        opTop--;
    }

    if (nfaTop < 0) {
        free(stack);
        free(operators);
        return createBasicNFA(' ');
    }

    NFA result = stack[nfaTop];

    free(stack);
    free(operators);

    return result;
}
