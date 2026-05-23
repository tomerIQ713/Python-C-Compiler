
#include "compiler_common.h"
/*
 * Loads the complete source file into memory.
 * Time complexity: O(n), where n is file size.
 */
char* readFile(char* fileName)
{
    FILE* file = fopen(fileName, "r");
    if (file == NULL)
        return NULL;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char* text = (char*)malloc(size + 1);
    if (text == NULL) {
        fclose(file);
        return NULL;
    }

    fread(text, sizeof(char), size, file);
    text[size] = '\0';

    fclose(file);
    return text;
}

