
#include "compiler_common.h"
/*
 * Initializes all compiler subsystems.
 */
void InitCompiler(ParserContext* parserContext, TranslationContext* translationContext)
{
    initAutomata();
    initErrorTable();
    initSymbolTable();
    initTranslationTable();
    initParserContext(parserContext);
    initTranslationContext(translationContext);
}

/*
 * Releases compiler resources before exit.
 */
void FreeSystem(char* sourceCode, ParserContext* parserContext,
    TranslationContext* translationContext)
{
    free(sourceCode);
    free(parserContext);
    free(translationContext);

    freeErrorTable();
    freeSymbolTable();
    freeTranslationTable();
    fclose(stdout);
}

/*
 * Checks whether a line already has an error.
 */
void checkForError(int lineNumber) {
    if (getErrorCode(lineNumber) != SUCCESS_CODE)
        return;
}

/*
 * Runs parsing and semantic analysis for one tokenized line.
 */
int ProcessTokenLine(TokenLine* tokenLine, ParserContext* parserContext)
{
    unsigned short errorCode;
    int result = SUCCESS_CODE;

    errorCode = parsingCheck(tokenLine, parserContext);
    if (errorCode != SUCCESS_CODE)
        result = GENERAL_ERROR;

    errorCode = semanticCheck(tokenLine);
    if (errorCode != SUCCESS_CODE)
        result = GENERAL_ERROR;

    return result;
}

