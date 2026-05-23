#include "compiler_common.h"

/*
 * computes the epsilon-closure of a set of NFA states.
 * O(S * T), where S is the number of states and T is the number of NFA transitions.
 */
void epsilonClosure(NFA* nfa, int stateSet[], int closure[])
{
    for (int i = 0; i < MAX_STATES; i++)
        closure[i] = stateSet[i];

    int changed = 1;

    while (changed) {
        changed = 0;

        for (int i = 0; i < nfa->transitionCount; i++) {
            NFATransition t = nfa->transitions[i];

            if (t.symbol == EPSILON &&
                closure[t.from] &&
                !closure[t.to]) {
                closure[t.to] = 1;
                changed = 1;
            }
        }
    }
}

/*
 * computes the NFA move operation for one input symbol.
 * O(T), where T is the number of NFA transitions.
 */
void moveNFA(NFA* nfa, int stateSet[], char symbol, int result[])
{
    for (int i = 0; i < MAX_STATES; i++)
        result[i] = 0;

    for (int i = 0; i < nfa->transitionCount; i++) {
        NFATransition t = nfa->transitions[i];

        if (stateSet[t.from] && t.symbol == symbol)
            result[t.to] = 1;
    }
}

int sameSet(int a[], int b[])
{
    for (int i = 0; i < MAX_STATES; i++) {
        if (a[i] != b[i])
            return 0;
    }

    return 1;
}

int findSet(int sets[][MAX_STATES], int count, int target[])
{
    for (int i = 0; i < count; i++) {
        if (sameSet(sets[i], target))
            return i;
    }

    return -1;
}

int isEmptySet(int set[])
{
    for (int i = 0; i < MAX_STATES; i++) {
        if (set[i])
            return 0;
    }

    return 1;
}

/*
 * converts an NFA into a DFA using Subset Construction.
 * O(D * A * (T + D*S)), where D is DFA states, A is alphabet size, T is NFA transitions, and S is MAX_STATES.
 */
DFA subsetConstruction(NFA nfa)
{
    DFA dfa;
    int dfaStates[MAX_STATES][MAX_STATES];
    int unmarked[MAX_STATES];

    dfa.stateCount = 0;
    dfa.startState = 0;
    dfa.acceptCount = 0;

    for (int i = 0; i < MAX_STATES; i++) {
        dfa.acceptStates[i] = 0;
        unmarked[i] = 0;

        for (int j = 0; j < MAX_SYMBOLS; j++)
            dfa.transitions[i][j] = -1;

        for (int j = 0; j < MAX_STATES; j++)
            dfaStates[i][j] = 0;
    }

    int startSet[MAX_STATES] = { 0 };
    int startClosure[MAX_STATES] = { 0 };

    startSet[nfa.startState] = 1;
    epsilonClosure(&nfa, startSet, startClosure);

    for (int i = 0; i < MAX_STATES; i++)
        dfaStates[0][i] = startClosure[i];

    dfa.stateCount = 1;
    unmarked[0] = 1;

    while (1) {
        int current = -1;

        for (int i = 0; i < dfa.stateCount; i++) {
            if (unmarked[i]) {
                current = i;
                unmarked[i] = 0;
                break;
            }
        }

        if (current == -1)
            break;

        for (int symbol = 1; symbol < MAX_SYMBOLS; symbol++) {
            int moveSet[MAX_STATES];
            int closureSet[MAX_STATES];

            moveNFA(&nfa, dfaStates[current], (char)symbol, moveSet);
            epsilonClosure(&nfa, moveSet, closureSet);

            if (isEmptySet(closureSet))
                continue;

            int existing = findSet(dfaStates, dfa.stateCount, closureSet);

            if (existing == -1) {
                if (dfa.stateCount >= MAX_STATES)
                    continue;

                existing = dfa.stateCount;

                for (int k = 0; k < MAX_STATES; k++)
                    dfaStates[existing][k] = closureSet[k];

                unmarked[existing] = 1;
                dfa.stateCount++;
            }

            dfa.transitions[current][symbol] = existing;
        }
    }

    for (int i = 0; i < dfa.stateCount; i++) {
        if (dfaStates[i][nfa.acceptState]) {
            dfa.acceptStates[i] = 1;
            dfa.acceptCount++;
        }
    }

    return dfa;
}