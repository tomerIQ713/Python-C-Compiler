
#include "compiler_common.h"

int parseExpression(Token tokens[], int start, int end);
int parseCondition(Token tokens[], int start, int end);
int parseSimpleExpression(Token tokens[], int start, int end);
int parseFunctionCallExpression(Token tokens[], int start, int end);

int pushParen(int stack[], int* top, int index);
int popParen(int stack[], int* top);
int parenthesesBalanced(Token tokens[], int start, int end);
int isMatchingOuterParentheses(Token tokens[], int start, int end);

int pushParen(int stack[], int* top, int index)
{
    if (*top >= MAX_TOKENS_PER_LINE - 1)
        return 0;

    (*top)++;
    stack[*top] = index;
    return 1;
}

int popParen(int stack[], int* top)
{
    if (*top < 0)
        return 0;

    (*top)--;
    return 1;
}

int parenthesesBalanced(Token tokens[], int start, int end)
{
    int stack[MAX_TOKENS_PER_LINE];
    int top = -1;

    for (int i = start; i <= end; i++) {
        if (tokens[i].type == TOKEN_LPAREN) {
            if (!pushParen(stack, &top, i))
                return 0;
        }
        else if (tokens[i].type == TOKEN_RPAREN) {
            if (!popParen(stack, &top))
                return 0;
        }
    }

    return top == -1;
}

int isMatchingOuterParentheses(Token tokens[], int start, int end)
{
    if (tokens[start].type != TOKEN_LPAREN ||
        tokens[end].type != TOKEN_RPAREN)
        return 0;

    int stack[MAX_TOKENS_PER_LINE];
    int top = -1;

    for (int i = start; i <= end; i++) {
        if (tokens[i].type == TOKEN_LPAREN) {
            if (!pushParen(stack, &top, i))
                return 0;
        }
        else if (tokens[i].type == TOKEN_RPAREN) {
            if (!popParen(stack, &top))
                return 0;

            if (top == -1 && i < end)
                return 0;
        }
    }

    return top == -1;
}

/*
 * Sets the indentation stack to base level and clears pending block state.
 * O(1).
 */
void initParserContext(ParserContext* context)
{
    context->top = 0;
    context->indentStack[0] = 0;
    context->waitingForIndentedBlock = 0;
    context->blockStarterLine = 0;
}

int currentIndent(ParserContext* context)
{
    return context->indentStack[context->top];
}

/*
 * Pushes a new indentation level.
 * O(1).
 */
void pushIndent(ParserContext* context, int indent)
{
    if (context->top < INDENT_STACK_SIZE - 1) {
        context->top++;
        context->indentStack[context->top] = indent;
    }
}

/*
 * Pops indentation levels until the requested level is reached.
 * O(d), where d is number of popped levels.
 */
int popIndentUntil(ParserContext* context, int indent)
{
    while (context->top > 0 && currentIndent(context) > indent)
        context->top--;

    return currentIndent(context) == indent;
}


/*
 * Adds a typed transition to a token-level parser automaton.
 * O(1).
 */
void addTokenTransition(TokenAutomaton* a, int from, TokenType type, int to)
{
    TokenTransition t;
    t.fromState = from;
    t.tokenType = type;
    t.toState = to;
    t.acceptAny = 0;
    a->transitions[a->transitionCount++] = t;
}

/*
 * Stores a transition that accepts any token type.
 * O(1).
 */
void addTokenTransitionAny(TokenAutomaton* a, int from, int to)
{
    TokenTransition t;
    t.fromState = from;
    t.tokenType = TOKEN_UNKNOWN;
    t.toState = to;
    t.acceptAny = 1;
    a->transitions[a->transitionCount++] = t;
}

/*
 * Runs a token-level automaton on a token sequence.
 * O(k * T), where k is token count and T is automaton transitions.
 */
int runTokenAutomaton(TokenAutomaton* a, Token tokens[], int count)
{
    int state = a->startState;

    for (int i = 0; i < count; i++) {
        int moved = 0;

        for (int j = 0; j < a->transitionCount; j++) {
            TokenTransition t = a->transitions[j];

            if (t.fromState == state &&
                !t.acceptAny &&
                t.tokenType == tokens[i].type) {
                state = t.toState;
                moved = 1;
                break;
            }
        }

        if (!moved) {
            for (int j = 0; j < a->transitionCount; j++) {
                TokenTransition t = a->transitions[j];

                if (t.fromState == state && t.acceptAny) {
                    state = t.toState;
                    moved = 1;
                    break;
                }
            }
        }

        if (!moved)
            return 0;
    }

    return state == a->acceptState;
}

/*
 * Runs a parser automaton with debug output.
 * O(k * T).
 */
int runNamedAutomaton(char* name, TokenAutomaton* a, Token tokens[], int count, int errorCode)
{
    printf("  AUTOMATA: trying %s\n", name);

    if (runTokenAutomaton(a, tokens, count)) {
        printf("  AUTOMATA: %s accepted\n", name);
        return SUCCESS_CODE;
    }

    printf("  AUTOMATA: %s rejected -> %s\n", name, errorCodeToString(errorCode));
    return errorCode;
}


/*
 * Checks whether a token can appear as a basic expression value.
 * O(1).
 */
int isExpressionToken(TokenType type)
{
    return type == TOKEN_IDENTIFIER ||
        type == TOKEN_NUMBER ||
        type == TOKEN_FLOAT_NUMBER ||
        type == TOKEN_STRING ||
        type == TOKEN_KEYWORD_TRUE ||
        type == TOKEN_KEYWORD_FALSE;
}

/*
 * Checks whether a token is a logical binary operator.
 * O(1).
 */
int isLogicalOperator(TokenType type)
{
    return type == TOKEN_KEYWORD_AND || type == TOKEN_KEYWORD_OR;
}

/*
 * Recognizes Python not token.
 * O(1).
 */
int isUnaryOperator(TokenType type)
{
    return type == TOKEN_KEYWORD_NOT;
}

/*
 * Adds transitions from start to accept for valid value tokens.
 * O(1).
 */
TokenAutomaton buildSingleValueExpressionAutomaton()
{
    TokenAutomaton a;
    a.startState = 0;
    a.acceptState = 1;
    a.transitionCount = 0;

    addTokenTransition(&a, 0, TOKEN_IDENTIFIER, 1);
    addTokenTransition(&a, 0, TOKEN_NUMBER, 1);
    addTokenTransition(&a, 0, TOKEN_FLOAT_NUMBER, 1);
    addTokenTransition(&a, 0, TOKEN_STRING, 1);
    addTokenTransition(&a, 0, TOKEN_KEYWORD_TRUE, 1);
    addTokenTransition(&a, 0, TOKEN_KEYWORD_FALSE, 1);

    return a;
}

/*
 * Builds a parser automaton for arithmetic expressions.
 * O(1).
 */
TokenAutomaton buildArithmeticExpressionAutomaton()
{
    TokenAutomaton a;
    a.startState = 0;
    a.acceptState = 1;
    a.transitionCount = 0;

    addTokenTransition(&a, 0, TOKEN_IDENTIFIER, 1);
    addTokenTransition(&a, 0, TOKEN_NUMBER, 1);
    addTokenTransition(&a, 0, TOKEN_FLOAT_NUMBER, 1);
    addTokenTransition(&a, 0, TOKEN_STRING, 1);
    addTokenTransition(&a, 0, TOKEN_KEYWORD_TRUE, 1);
    addTokenTransition(&a, 0, TOKEN_KEYWORD_FALSE, 1);

    addTokenTransition(&a, 1, TOKEN_OPERATOR_PLUS, 2);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_MINUS, 2);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_MULTIPLY, 2);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_DIVIDE, 2);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_MODULO, 2);

    addTokenTransition(&a, 2, TOKEN_IDENTIFIER, 1);
    addTokenTransition(&a, 2, TOKEN_NUMBER, 1);
    addTokenTransition(&a, 2, TOKEN_FLOAT_NUMBER, 1);
    addTokenTransition(&a, 2, TOKEN_STRING, 1);
    addTokenTransition(&a, 2, TOKEN_KEYWORD_TRUE, 1);
    addTokenTransition(&a, 2, TOKEN_KEYWORD_FALSE, 1);

    return a;
}

/*
 * Builds a parser automaton for comparison conditions.
 * O(1).
 */
TokenAutomaton buildComparisonConditionAutomaton()
{
    TokenAutomaton a;
    a.startState = 0;
    a.acceptState = 3;
    a.transitionCount = 0;

    addTokenTransition(&a, 0, TOKEN_IDENTIFIER, 1);
    addTokenTransition(&a, 0, TOKEN_NUMBER, 1);
    addTokenTransition(&a, 0, TOKEN_FLOAT_NUMBER, 1);
    addTokenTransition(&a, 0, TOKEN_STRING, 1);
    addTokenTransition(&a, 0, TOKEN_KEYWORD_TRUE, 1);
    addTokenTransition(&a, 0, TOKEN_KEYWORD_FALSE, 1);

    addTokenTransition(&a, 1, TOKEN_OPERATOR_EQUAL, 2);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_NOT_EQUAL, 2);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_LTE, 2);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_GTE, 2);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_LT, 2);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_GT, 2);

    addTokenTransition(&a, 2, TOKEN_IDENTIFIER, 3);
    addTokenTransition(&a, 2, TOKEN_NUMBER, 3);
    addTokenTransition(&a, 2, TOKEN_FLOAT_NUMBER, 3);
    addTokenTransition(&a, 2, TOKEN_STRING, 3);
    addTokenTransition(&a, 2, TOKEN_KEYWORD_TRUE, 3);
    addTokenTransition(&a, 2, TOKEN_KEYWORD_FALSE, 3);

    return a;
}


/*
 * Builds a parser automaton for function definitions.
 * O(1).
 */
TokenAutomaton buildFunctionDefAutomaton()
{
    TokenAutomaton a;
    a.startState = 0;
    a.acceptState = 8;
    a.transitionCount = 0;

    addTokenTransition(&a, 0, TOKEN_KEYWORD_DEF, 1);
    addTokenTransition(&a, 1, TOKEN_IDENTIFIER, 2);
    addTokenTransition(&a, 2, TOKEN_LPAREN, 3);

    addTokenTransition(&a, 3, TOKEN_RPAREN, 7);
    addTokenTransition(&a, 3, TOKEN_IDENTIFIER, 4);

    addTokenTransition(&a, 4, TOKEN_COMMA, 3);
    addTokenTransition(&a, 4, TOKEN_RPAREN, 7);
    addTokenTransition(&a, 4, TOKEN_COLON, 5);

    addTokenTransition(&a, 5, TOKEN_KEYWORD_STR, 6);
    addTokenTransition(&a, 5, TOKEN_KEYWORD_INT, 6);
    addTokenTransition(&a, 5, TOKEN_KEYWORD_FLOAT, 6);

    addTokenTransition(&a, 6, TOKEN_COMMA, 3);
    addTokenTransition(&a, 6, TOKEN_RPAREN, 7);

    addTokenTransition(&a, 7, TOKEN_COLON, 8);

    return a;
}

/*
 * Builds a parser automaton for function calls.
 * O(1).
 */
TokenAutomaton buildFunctionCallAutomaton()
{
    TokenAutomaton a;
    a.startState = 0;
    a.acceptState = 4;
    a.transitionCount = 0;


    addTokenTransition(&a, 0, TOKEN_IDENTIFIER, 1);
    addTokenTransition(&a, 1, TOKEN_LPAREN, 2);

    /* no arguments */
    addTokenTransition(&a, 2, TOKEN_RPAREN, 4);

    /* normal argument */
    addTokenTransition(&a, 2, TOKEN_IDENTIFIER, 3);
    addTokenTransition(&a, 2, TOKEN_NUMBER, 3);
    addTokenTransition(&a, 2, TOKEN_FLOAT_NUMBER, 3);
    addTokenTransition(&a, 2, TOKEN_STRING, 3);
    addTokenTransition(&a, 2, TOKEN_KEYWORD_TRUE, 3);
    addTokenTransition(&a, 2, TOKEN_KEYWORD_FALSE, 3);

    /* negative number argument: - 4 */
    addTokenTransition(&a, 2, TOKEN_OPERATOR_MINUS, 5);
    addTokenTransition(&a, 5, TOKEN_NUMBER, 3);
    addTokenTransition(&a, 5, TOKEN_FLOAT_NUMBER, 3);

    /* after argument */
    addTokenTransition(&a, 3, TOKEN_COMMA, 2);
    addTokenTransition(&a, 3, TOKEN_RPAREN, 4);

    return a;
}


/*
 * Builds a parser automaton for for-in-range statements.
 * O(1).
 */
TokenAutomaton buildForRangeAutomaton()
{
    TokenAutomaton a;
    a.startState = 0;
    a.acceptState = 20;
    a.transitionCount = 0;

    addTokenTransition(&a, 0, TOKEN_KEYWORD_FOR, 1);
    addTokenTransition(&a, 1, TOKEN_IDENTIFIER, 2);
    addTokenTransition(&a, 2, TOKEN_KEYWORD_IN, 3);
    addTokenTransition(&a, 3, TOKEN_KEYWORD_RANGE, 4);
    addTokenTransition(&a, 4, TOKEN_LPAREN, 5);

    /* first value */
    addTokenTransition(&a, 5, TOKEN_NUMBER, 6);
    addTokenTransition(&a, 5, TOKEN_IDENTIFIER, 6);

    /* range(stop) */
    addTokenTransition(&a, 6, TOKEN_RPAREN, 18);

    /* range(start, ...) */
    addTokenTransition(&a, 6, TOKEN_COMMA, 7);

    /* second value */
    addTokenTransition(&a, 7, TOKEN_NUMBER, 8);
    addTokenTransition(&a, 7, TOKEN_IDENTIFIER, 8);

    /* range(start, stop) */
    addTokenTransition(&a, 8, TOKEN_RPAREN, 18);

    /* range(start, stop, step) */
    addTokenTransition(&a, 8, TOKEN_COMMA, 9);

    /* step */
    addTokenTransition(&a, 9, TOKEN_NUMBER, 10);
    addTokenTransition(&a, 9, TOKEN_IDENTIFIER, 10);

    /* negative step */
    addTokenTransition(&a, 9, TOKEN_OPERATOR_MINUS, 11);
    addTokenTransition(&a, 11, TOKEN_NUMBER, 10);

    addTokenTransition(&a, 10, TOKEN_RPAREN, 18);

    /* final */
    addTokenTransition(&a, 18, TOKEN_COLON, 20);

    return a;
}


/*
 * Builds a parser skeleton for assignments.
 * O(1).
 */
TokenAutomaton buildAssignmentSkeletonAutomaton()
{
    TokenAutomaton a;
    a.startState = 0;
    a.acceptState = 3;
    a.transitionCount = 0;

    addTokenTransition(&a, 0, TOKEN_IDENTIFIER, 1);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_ASSIGN, 2);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_PLUS_ASSIGN, 2);
    addTokenTransition(&a, 1, TOKEN_OPERATOR_MINUS_ASSIGN, 2);
    addTokenTransitionAny(&a, 2, 3);
    addTokenTransitionAny(&a, 3, 3);

    return a;
}

/*
 * Builds a parser skeleton for colon-ended block statements.
 * O(1).
 */
TokenAutomaton buildBlockSkeletonAutomaton(TokenType keyword)
{
    TokenAutomaton a;
    a.startState = 0;
    a.acceptState = 4;
    a.transitionCount = 0;

    addTokenTransition(&a, 0, keyword, 1);
    addTokenTransitionAny(&a, 1, 2);
    addTokenTransitionAny(&a, 2, 2);
    addTokenTransition(&a, 2, TOKEN_COLON, 4);

    return a;
}

/*
 * Builds a parser automaton for else blocks.
 * O(1).
 */
TokenAutomaton buildElseAutomaton()
{
    TokenAutomaton a;
    a.startState = 0;
    a.acceptState = 2;
    a.transitionCount = 0;

    addTokenTransition(&a, 0, TOKEN_KEYWORD_ELSE, 1);
    addTokenTransition(&a, 1, TOKEN_COLON, 2);

    return a;
}

/*
 * Builds a parser skeleton for print calls.
 * O(1).
 */
TokenAutomaton buildPrintSkeletonAutomaton()
{
    TokenAutomaton a;
    a.startState = 0;
    a.acceptState = 5;
    a.transitionCount = 0;

    addTokenTransition(&a, 0, TOKEN_KEYWORD_PRINT, 1);
    addTokenTransition(&a, 1, TOKEN_LPAREN, 2);
    addTokenTransitionAny(&a, 2, 3);
    addTokenTransitionAny(&a, 3, 3);
    addTokenTransition(&a, 3, TOKEN_RPAREN, 5);

    return a;
}

/*
 * Builds a parser skeleton for return statements.
 * O(1).
 */
TokenAutomaton buildReturnSkeletonAutomaton()
{
    TokenAutomaton a;
    a.startState = 0;
    a.acceptState = 1;
    a.transitionCount = 0;

    addTokenTransition(&a, 0, TOKEN_KEYWORD_RETURN, 1);
    addTokenTransitionAny(&a, 1, 1);

    return a;
}

/*
 * Validates a function-call expression.
 * O(k) worst case due to nested expression validation.
 */
int parseFunctionCallExpression(Token tokens[], int start, int end)
{
    if (tokens[start].type != TOKEN_IDENTIFIER)
        return 0;

    if (start + 1 > end || tokens[start + 1].type != TOKEN_LPAREN)
        return 0;

    if (tokens[end].type != TOKEN_RPAREN)
        return 0;

    int argStart = start + 2;
    int depth = 0;

    if (argStart == end)
        return 1;

    for (int i = argStart; i <= end - 1; i++) {
        if (tokens[i].type == TOKEN_LPAREN)
            depth++;

        else if (tokens[i].type == TOKEN_RPAREN)
            depth--;

        if ((tokens[i].type == TOKEN_COMMA && depth == 0) || i == end - 1) {
            int argEnd = (tokens[i].type == TOKEN_COMMA) ? i - 1 : i;

            if (!parseExpression(tokens, argStart, argEnd))
                return 0;

            argStart = i + 1;
        }
    }

    return 1;
}

/*
 * Validates a simple expression segment.
 * O(k) worst case with nested calls.
 */
int parseSimpleExpression(Token tokens[], int start, int end)
{
    if (start > end)
        return 0;

    if (start + 1 <= end &&
        tokens[start].type == TOKEN_IDENTIFIER &&
        tokens[start + 1].type == TOKEN_LPAREN) {
        printf("  AUTOMATA: expression uses FUNCTION_CALL_AUTOMATON\n");
        return parseFunctionCallExpression(tokens, start, end);
    }

    if (isUnaryOperator(tokens[start].type))
        start++;

    if (start > end)
        return 0;

    TokenAutomaton singleValue = buildSingleValueExpressionAutomaton();
    if (runTokenAutomaton(&singleValue, &tokens[start], end - start + 1)) {
        printf("  AUTOMATA: expression accepted by SINGLE_VALUE_EXPRESSION_AUTOMATON\n");
        return 1;
    }

    TokenAutomaton arithmetic = buildArithmeticExpressionAutomaton();
    if (runTokenAutomaton(&arithmetic, &tokens[start], end - start + 1)) {
        printf("  AUTOMATA: expression accepted by ARITHMETIC_EXPRESSION_AUTOMATON\n");
        return 1;
    }

    printf("  AUTOMATA: expression rejected\n");
    return 0;
}

/*
 * Validates a general expression.
 * O(k)
 */
int parseExpression(Token tokens[], int start, int end)
{
    if (start > end)
        return 0;

    if (!parenthesesBalanced(tokens, start, end))
        return 0;

    /*
     * Parenthesized expression:
     * ( expression )
     */
    if (isMatchingOuterParentheses(tokens, start, end)) {
        if (parseExpression(tokens, start + 1, end - 1)) {
            printf("  AUTOMATA: expression accepted by PARENTHESIZED_EXPRESSION_AUTOMATON\n");
            return 1;
        }
    }

    int stack[MAX_TOKENS_PER_LINE];
    int top = -1;

    /*
     * Lowest precedence: + and -
     */
    for (int i = end; i >= start; i--) {
        if (tokens[i].type == TOKEN_RPAREN) {
            pushParen(stack, &top, i);
        }
        else if (tokens[i].type == TOKEN_LPAREN) {
            popParen(stack, &top);
        }

        if (top == -1 &&
            (tokens[i].type == TOKEN_OPERATOR_PLUS ||
                tokens[i].type == TOKEN_OPERATOR_MINUS)) {

            if (parseExpression(tokens, start, i - 1) &&
                parseExpression(tokens, i + 1, end)) {
                printf("  AUTOMATA: expression accepted by ARITHMETIC_EXPRESSION_AUTOMATON\n");
                return 1;
            }
        }
    }

    top = -1;

    /*
     * Higher precedence: *, / and %
     */
    for (int i = end; i >= start; i--) {
        if (tokens[i].type == TOKEN_RPAREN) {
            pushParen(stack, &top, i);
        }
        else if (tokens[i].type == TOKEN_LPAREN) {
            popParen(stack, &top);
        }

        if (top == -1 &&
            (tokens[i].type == TOKEN_OPERATOR_MULTIPLY ||
                tokens[i].type == TOKEN_OPERATOR_DIVIDE ||
                tokens[i].type == TOKEN_OPERATOR_MODULO)) {

            if (parseExpression(tokens, start, i - 1) &&
                parseExpression(tokens, i + 1, end)) {
                printf("  AUTOMATA: expression accepted by ARITHMETIC_EXPRESSION_AUTOMATON\n");
                return 1;
            }
        }
    }

    /*
     * Function call expression:
     * func(...)
     */
    if (tokens[start].type == TOKEN_IDENTIFIER &&
        start + 1 <= end &&
        tokens[start + 1].type == TOKEN_LPAREN) {

        if (parseFunctionCallExpression(tokens, start, end)) {
            printf("  AUTOMATA: expression uses FUNCTION_CALL_AUTOMATON\n");
            return 1;
        }
    }

    /*
     * identifier / number / float / string / True / False
     */
    if (start == end) {
        if (tokens[start].type == TOKEN_IDENTIFIER ||
            tokens[start].type == TOKEN_NUMBER ||
            tokens[start].type == TOKEN_FLOAT_NUMBER ||
            tokens[start].type == TOKEN_STRING ||
            tokens[start].type == TOKEN_KEYWORD_TRUE ||
            tokens[start].type == TOKEN_KEYWORD_FALSE) {

            printf("  AUTOMATA: expression accepted by SINGLE_VALUE_EXPRESSION_AUTOMATON\n");
            return 1;
        }
    }

    return 0;
}

/*
 * Validates a boolean/comparison condition.
 * O(k)
 */
int parseCondition(Token tokens[], int start, int end)
{
    if (start > end)
        return 0;

    int segmentStart = start;

    for (int i = start; i <= end; i++) {
        if (isLogicalOperator(tokens[i].type)) {
            if (!parseCondition(tokens, segmentStart, i - 1))
                return 0;

            printf("  AUTOMATA: logical condition operator accepted\n");
            segmentStart = i + 1;
        }
    }

    int comparisonIndex = -1;

    for (int i = segmentStart; i <= end; i++) {
        if (tokens[i].type == TOKEN_OPERATOR_EQUAL ||
            tokens[i].type == TOKEN_OPERATOR_NOT_EQUAL ||
            tokens[i].type == TOKEN_OPERATOR_LTE ||
            tokens[i].type == TOKEN_OPERATOR_GTE ||
            tokens[i].type == TOKEN_OPERATOR_LT ||
            tokens[i].type == TOKEN_OPERATOR_GT) {
            comparisonIndex = i;
            break;
        }
    }

    if (comparisonIndex != -1) {
        if (!parseExpression(tokens, segmentStart, comparisonIndex - 1))
            return 0;

        if (!parseExpression(tokens, comparisonIndex + 1, end))
            return 0;

        printf("  AUTOMATA: condition accepted by EXPRESSION_COMPARE_EXPRESSION_AUTOMATON\n");
        return 1;
    }

    if (parseExpression(tokens, segmentStart, end)) {
        printf("  AUTOMATA: condition accepted by EXPRESSION_AUTOMATON\n");
        return 1;
    }

    printf("  AUTOMATA: condition rejected\n");
    return 0;
}


/*
 * Validates an assignment statement.
 * O(k)
 */
int parseAssignment(TokenLine* line)
{
    TokenAutomaton a = buildAssignmentSkeletonAutomaton();

    int result = runNamedAutomaton(
        "ASSIGNMENT_AUTOMATON: identifier -> assignment_operator -> expression",
        &a,
        line->tokens,
        line->count,
        ERROR_SYNTAX_EXPECTED_ASSIGNMENT
    );

    if (result != SUCCESS_CODE)
        return result;

    if (!parseExpression(line->tokens, 2, line->count - 1))
        return ERROR_SYNTAX_EXPECTED_EXPRESSION;

    return SUCCESS_CODE;
}

/*
 * Validates if/elif/while block statements.
 */
int parseIfOrWhile(TokenLine* line)
{
    TokenAutomaton a;

    if (line->tokens[0].type == TOKEN_KEYWORD_IF)
        a = buildBlockSkeletonAutomaton(TOKEN_KEYWORD_IF);
    else if (line->tokens[0].type == TOKEN_KEYWORD_ELIF)
        a = buildBlockSkeletonAutomaton(TOKEN_KEYWORD_ELIF);
    else
        a = buildBlockSkeletonAutomaton(TOKEN_KEYWORD_WHILE);

    int result = runNamedAutomaton(
        "BLOCK_AUTOMATON: keyword -> expression -> colon",
        &a,
        line->tokens,
        line->count,
        ERROR_SYNTAX_EXPECTED_COLON
    );

    if (result != SUCCESS_CODE)
        return result;

    if (!parseCondition(line->tokens, 1, line->count - 2))
        return ERROR_SYNTAX_EXPECTED_EXPRESSION;

    return SUCCESS_CODE;
}

/*
 * Purpose: Validates a for-in-range loop statement.
 * O(k * T), where T is automaton transitions.
 */
int parseFor(TokenLine* line)
{
    printf("  for -> identifier -> in -> range(expressions) -> colon\n");

    if (line->count < 8 ||
        line->tokens[0].type != TOKEN_KEYWORD_FOR ||
        line->tokens[1].type != TOKEN_IDENTIFIER ||
        line->tokens[2].type != TOKEN_KEYWORD_IN ||
        line->tokens[3].type != TOKEN_KEYWORD_RANGE ||
        line->tokens[4].type != TOKEN_LPAREN ||
        line->tokens[line->count - 2].type != TOKEN_RPAREN ||
        line->tokens[line->count - 1].type != TOKEN_COLON) {

        printf("  FOR rejected -> %s\n",
            errorCodeToString(ERROR_SYNTAX_INVALID_STATEMENT));

        return ERROR_SYNTAX_INVALID_STATEMENT;
    }

    int argStart = 5;
    int argCount = 0;
    int depth = 0;

    for (int i = 5; i <= line->count - 2; i++) {
        if (line->tokens[i].type == TOKEN_LPAREN)
            depth++;

        else if (line->tokens[i].type == TOKEN_RPAREN)
            depth--;

        if ((line->tokens[i].type == TOKEN_COMMA && depth == 0) ||
            i == line->count - 2) {

            int argEnd = (line->tokens[i].type == TOKEN_COMMA) ? i - 1 : i - 1;

            if (!parseExpression(line->tokens, argStart, argEnd)) {
                printf("  AUTOMATA: FOR_AUTOMATON rejected -> invalid range expression\n");
                return ERROR_SYNTAX_EXPECTED_EXPRESSION;
            }

            argCount++;
            argStart = i + 1;
        }
    }

    if (argCount < 1 || argCount > 3) {
        printf("  AUTOMATA: FOR_AUTOMATON rejected -> range expects 1 to 3 arguments\n");
        return ERROR_SYNTAX_INVALID_STATEMENT;
    }

    printf("  AUTOMATA: FOR_AUTOMATON accepted\n");
    return SUCCESS_CODE;
}

/*
 * Validates an else statement.
 * Time complexity: O(k * T).
 */
int parseElse(TokenLine* line)
{
    TokenAutomaton a = buildElseAutomaton();

    return runNamedAutomaton(
        "ELSE_AUTOMATON: else -> colon",
        &a,
        line->tokens,
        line->count,
        ERROR_SYNTAX_EXPECTED_COLON
    );
}

/*
 * Validates a print statement.
 */
int parsePrint(TokenLine* line)
{
    printf("  AUTOMATA: trying PRINT_AUTOMATON: print -> ( expression )\n");

    if (line->count < 4 ||
        line->tokens[0].type != TOKEN_KEYWORD_PRINT ||
        line->tokens[1].type != TOKEN_LPAREN ||
        line->tokens[line->count - 1].type != TOKEN_RPAREN) {

        printf("  AUTOMATA: PRINT_AUTOMATON rejected -> %s\n",
            errorCodeToString(ERROR_SYNTAX_EXPECTED_PARENTHESES));

        return ERROR_SYNTAX_EXPECTED_PARENTHESES;
    }

    if (!parseExpression(line->tokens, 2, line->count - 2)) {
        printf("  AUTOMATA: PRINT_AUTOMATON rejected -> %s\n",
            errorCodeToString(ERROR_SYNTAX_EXPECTED_EXPRESSION));

        return ERROR_SYNTAX_EXPECTED_EXPRESSION;
    }

    printf("  AUTOMATA: PRINT_AUTOMATON accepted\n");
    return SUCCESS_CODE;
}

/*
 * Validates a function definition line.
 */
int parseFunctionDef(TokenLine* line)
{
    TokenAutomaton a = buildFunctionDefAutomaton();

    return runNamedAutomaton(
        "FUNCTION_DEF_AUTOMATON: def -> identifier -> parameters -> colon",
        &a,
        line->tokens,
        line->count,
        ERROR_SYNTAX_INVALID_FUNCTION
    );
}

/*
 * Validates a standalone function call statement.
 */
int parseFunctionCall(TokenLine* line)
{
    TokenAutomaton a = buildFunctionCallAutomaton();

    return runNamedAutomaton(
        "FUNCTION_CALL_AUTOMATON: identifier -> arguments",
        &a,
        line->tokens,
        line->count,
        ERROR_SYNTAX_INVALID_STATEMENT
    );
}

/*
 * Validates a return statement.
 */
int parseReturn(TokenLine* line)
{
    if (line->indentLevel == 0) {
        printf("  Parsing error: return outside function\n");
        return ERROR_SYNTAX_EXPECTED_INDENT;
    }

    TokenAutomaton a = buildReturnSkeletonAutomaton();

    int result = runNamedAutomaton(
        "RETURN_AUTOMATON: return -> optional_expression",
        &a,
        line->tokens,
        line->count,
        ERROR_SYNTAX_EXPECTED_EXPRESSION
    );

    if (result != SUCCESS_CODE)
        return result;

    if (line->count == 1)
        return SUCCESS_CODE;

    if (!parseExpression(line->tokens, 1, line->count - 1))
        return ERROR_SYNTAX_EXPECTED_EXPRESSION;

    return SUCCESS_CODE;
}

unsigned short IsIndentToken(TokenLine* line) {
    return line->tokens[0].type == TOKEN_KEYWORD_IF ||
        line->tokens[0].type == TOKEN_KEYWORD_ELIF ||
        line->tokens[0].type == TOKEN_KEYWORD_ELSE ||
        line->tokens[0].type == TOKEN_KEYWORD_WHILE ||
        line->tokens[0].type == TOKEN_KEYWORD_FOR ||
        line->tokens[0].type == TOKEN_KEYWORD_DEF;
}

int handleIndentationRules(TokenLine* line, ParserContext* context)
{
    if (!context->waitingForIndentedBlock &&
        context->top == 0 &&
        line->indentLevel > 0)
    {
        printf("  Parsing error: unexpected indentation\n");
        addErrorCode(line->lineNumber, ERROR_SYNTAX_UNEXPECTED_INDENT);
        return ERROR_SYNTAX_UNEXPECTED_INDENT;
    }

    if (context->waitingForIndentedBlock)
    {
        if (line->indentLevel <= currentIndent(context))
        {
            printf("  Parsing error: expected indented block after line %d\n",
                context->blockStarterLine);

            addErrorCode(line->lineNumber, ERROR_SYNTAX_EXPECTED_INDENT);
            context->waitingForIndentedBlock = 0;
            return ERROR_SYNTAX_EXPECTED_INDENT;
        }

        pushIndent(context, line->indentLevel);
        context->waitingForIndentedBlock = 0;
    }
    else
    {
        if (line->indentLevel > currentIndent(context))
        {
            printf("  Parsing error: unexpected indentation\n");
            addErrorCode(line->lineNumber, ERROR_SYNTAX_UNEXPECTED_INDENT);
            return ERROR_SYNTAX_UNEXPECTED_INDENT;
        }

        if (!popIndentUntil(context, line->indentLevel))
        {
            printf("  Parsing error: inconsistent indentation\n");
            addErrorCode(line->lineNumber, ERROR_SYNTAX_INCONSISTENT_INDENT);
            return ERROR_SYNTAX_INCONSISTENT_INDENT;
        }
    }

    return SUCCESS_CODE;
}

void handleIndent(TokenLine* line, ParserContext* context) {
    if (IsIndentToken(line)) {
        context->waitingForIndentedBlock = ONE;
        context->blockStarterLine = line->lineNumber;
    }
}

void handleError(TokenLine* line, unsigned short errorCode) {
    printf("  Parsing error: %s\n", errorCodeToString(errorCode));
    addErrorCode(line->lineNumber, errorCode);
}





/*
 * Chooses the correct parser routine for a tokenized statement.
 */
int parseStatement(TokenLine* line)
{
    if (line->count == ZERO)
        return SUCCESS_CODE;

    if (line->tokens[ZERO].type == TOKEN_UNKNOWN)
        return ERROR_UNKNOWN_TOKEN;

    switch (line->tokens[ZERO].type)
    {
    case TOKEN_IDENTIFIER:
        if (line->count >= TWO && line->tokens[ONE].type == TOKEN_LPAREN)
            return parseFunctionCall(line);
        return parseAssignment(line);

    case TOKEN_KEYWORD_IF:
    case TOKEN_KEYWORD_ELIF:
    case TOKEN_KEYWORD_WHILE:
        return parseIfOrWhile(line);

    case TOKEN_KEYWORD_FOR:
        return parseFor(line);

    case TOKEN_KEYWORD_ELSE:
        return parseElse(line);

    case TOKEN_KEYWORD_PRINT:
        return parsePrint(line);

    case TOKEN_KEYWORD_DEF:
        return parseFunctionDef(line);

    case TOKEN_KEYWORD_RETURN:
        return parseReturn(line);

    default:
        return ERROR_SYNTAX_INVALID_STATEMENT;
    }
}

/*
 * Performs full parser validation for one tokenized line.
 * O(k^2 + d), where k is tokens and d is indentation changes.
 */
int parsingCheck(TokenLine* line, ParserContext* context)
{
    if (line->count == ZERO)
        return SUCCESS_CODE;

    int indentResultCode = handleIndentationRules(line, context);
    if (indentResultCode != SUCCESS_CODE)
        return indentResultCode;

    unsigned short errorCode = parseStatement(line);

    if (errorCode != SUCCESS_CODE) {
        handleError(line, errorCode);
        return errorCode;
    }

    handleIndent(line, context);

    return SUCCESS_CODE;
}
