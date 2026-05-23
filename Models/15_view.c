
#include "compiler_common.h"
/*
 * Loads the input source file through the view layer.
 */
char* loadInputFile(char* fileName)
{
    return readFile(fileName);
}

/*
 * Shows a file-open error message to the user.
 */
void viewShowFileError(char* fileName)
{
    printf("Could not open %s\n", fileName);
}

/*
 * Shows a memory-allocation error message.
 */
void viewShowMemoryError()
{
    printf("Memory error\n");
}

/*
 * Displays final compiler debug/results information.
 */
void ShowInfo(int totalLines)
{
    printErrorTable(totalLines);
    printSymbolTable();
    printTranslationTable();
}

/*
 * Saves generated C code into an output file.
 */
void SaveOutput(char* fileName, char* globalOutput, char* cOutput)
{
    FILE* file = fopen(fileName, "w");

    if (file == NULL) {
        printf("Could not create %s\n", fileName);
        return;
    }

    fprintf(file, "#include <stdio.h>\n\n");
    fprintf(file, "%s", globalOutput);
    fprintf(file, "\nint main()\n{\n");
    fprintf(file, "%s", cOutput);
    fprintf(file, "    return 0;\n");
    fprintf(file, "}\n");

    fclose(file);

    printf("\nC translation was saved to %s\n", fileName);
}

