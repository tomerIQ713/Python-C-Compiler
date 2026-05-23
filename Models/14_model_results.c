
#include "compiler_common.h"
/*
 * Prints the final error result for every source line.
 */
void printErrorTable(int totalLines)
{
    printf("\n\n--- Error Hash Table Results ---\n");

    for (int i = 1; i <= totalLines; i++) {
        int code = getErrorCode(i);

        printf("Line %d -> Code %d (%s)\n",
            i,
            code,
            errorCodeToString(code));
    }
}
