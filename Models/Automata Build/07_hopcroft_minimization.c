#include "compiler_common.h"

/*
 * checks whether two DFA states are equivalent under the current partition.
 * O(A), where A is MAX_SYMBOLS.
 */
static int sameGroupByTransitions(DFA* dfa, int a, int b, int group[])
{
    if (dfa->acceptStates[a] != dfa->acceptStates[b])
        return 0;

    for (int symbol = 1; symbol < MAX_SYMBOLS; symbol++) {
        int nextA = dfa->transitions[a][symbol];
        int nextB = dfa->transitions[b][symbol];

        if (nextA == -1 && nextB == -1)
            continue;

        if (nextA == -1 || nextB == -1)
            return 0;

        if (group[nextA] != group[nextB])
            return 0;
    }

    return 1;
}

/*
 * minimizes a DFA by repeatedly refining groups of equivalent states.
 * O(I * D^2 * A), where I is the number of refinement iterations, D is DFA states, and A is alphabet size.
 */
DFA hopcroftMinimization(DFA dfa)
{
    int group[MAX_STATES];

    for (int i = 0; i < dfa.stateCount; i++) {
        if (dfa.acceptStates[i])
            group[i] = 1;
        else
            group[i] = 0;
    }

    int changed = 1;

    while (changed) {
        changed = 0;

        int newGroup[MAX_STATES];

        for (int i = 0; i < MAX_STATES; i++)
            newGroup[i] = -1;

        int groupCount = 0;

        for (int i = 0; i < dfa.stateCount; i++) {
            if (newGroup[i] != -1)
                continue;

            newGroup[i] = groupCount;

            for (int j = i + 1; j < dfa.stateCount; j++) {
                if (newGroup[j] == -1 &&
                    sameGroupByTransitions(&dfa, i, j, group)) {
                    newGroup[j] = groupCount;
                }
            }

            groupCount++;
        }

        for (int i = 0; i < dfa.stateCount; i++) {
            if (group[i] != newGroup[i]) {
                changed = 1;
                break;
            }
        }

        for (int i = 0; i < dfa.stateCount; i++)
            group[i] = newGroup[i];
    }

    DFA minimized;

    minimized.stateCount = 0;
    minimized.startState = group[dfa.startState];
    minimized.acceptCount = 0;

    for (int i = 0; i < MAX_STATES; i++) {
        minimized.acceptStates[i] = 0;

        for (int j = 0; j < MAX_SYMBOLS; j++)
            minimized.transitions[i][j] = -1;
    }

    int maxGroup = 0;

    for (int i = 0; i < dfa.stateCount; i++) {
        if (group[i] > maxGroup)
            maxGroup = group[i];
    }

    minimized.stateCount = maxGroup + 1;

    for (int i = 0; i < dfa.stateCount; i++) {
        int g = group[i];

        if (dfa.acceptStates[i])
            minimized.acceptStates[g] = 1;

        for (int symbol = 1; symbol < MAX_SYMBOLS; symbol++) {
            int next = dfa.transitions[i][symbol];

            if (next != -1)
                minimized.transitions[g][symbol] = group[next];
        }
    }

    for (int i = 0; i < minimized.stateCount; i++) {
        if (minimized.acceptStates[i])
            minimized.acceptCount++;
    }

    return minimized;
}

/*
 * validates that a regex contains only supported syntax before building automata.
 * O(n), where n is regex length.
 */
int isSafeThompsonRegex(char* regex)
{
    if (regex == NULL || regex[0] == '\0')
        return 0;

    for (int i = 0; regex[i] != '\0'; i++) {
        char c = regex[i];

        if (isRegexCharacter(c) ||
            c == '(' ||
            c == ')' ||
            c == '|' ||
            c == '*' ||
            c == '@') {
            continue;
        }

        if (c == '\\' && regex[i + 1] != '\0') {
            i++;
            continue;
        }

        return 0;
    }

    return 1;
}

/*
 * builds the final minimized DFA used by the lexer from a regex.
 * O(TC + SC + HM), where TC is Thompson cost, SC is Subset Construction cost, and HM is minimization cost.
 */
DFA buildAutomatonFromRegex(char* regex)
{
    DFA empty;

    empty.stateCount = 0;
    empty.startState = 0;
    empty.acceptCount = 0;

    for (int i = 0; i < MAX_STATES; i++) {
        empty.acceptStates[i] = 0;

        for (int j = 0; j < MAX_SYMBOLS; j++)
            empty.transitions[i][j] = -1;
    }

    if (!isSafeThompsonRegex(regex)) {
        printf("Skipped invalid regex: %s\n", regex);
        return empty;
    }

    NFA nfa = thompsonConstruction(regex);
    DFA dfa = subsetConstruction(nfa);
    DFA minimized = hopcroftMinimization(dfa);

    printf("Regex: %s\n", regex);
    printf("NFA states: %d\n", nfa.stateCount);
    printf("DFA states: %d\n", dfa.stateCount);
    printf("Minimized DFA states: %d\n\n", minimized.stateCount);

    return minimized;
}
