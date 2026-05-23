
/*
 * full compiler program.
 * O(n) Time Complexity
*/


#include "compiler_common.h"

#define EOD '\0'

int main()
{
    FILE* logFile = freopen("compilation_logs.txt", "w", stdout);
    setvbuf(stdout, NULL, _IONBF, ZERO);

    ParserContext* parserContext = (ParserContext*)malloc(sizeof(ParserContext));

    TranslationContext* translationContext =
        (TranslationContext*)malloc(sizeof(TranslationContext));

    char* sourceCode = loadInputFile("input.txt"); 

    if (translationContext == NULL || parserContext == NULL || sourceCode == NULL) {
        return ONE;
    }

    InitCompiler(parserContext, translationContext);

    unsigned int index = ZERO;
    unsigned int lineNumber = ONE;

    unsigned short errorCode;

    while (sourceCode[index] != EOD) {
        char* line = GetNextLine(sourceCode, &index);

        TokenLine tokenLine = processLineToTokens(line, lineNumber);


        errorCode = ProcessTokenLine(&tokenLine, parserContext);

        errorCode == SUCCESS_CODE ?
            translateTokenLineToC(&tokenLine, translationContext) : ZERO;

        free(line);
        lineNumber++;
    }

    closeAllTranslationBlocks(translationContext);

    SaveOutput(
        "output.c",
        translationContext->globalOutput,
        getCOutput(translationContext)
    );

    ShowInfo(lineNumber - ONE);

    FreeSystem(sourceCode, parserContext, translationContext);


    return ZERO;
}
