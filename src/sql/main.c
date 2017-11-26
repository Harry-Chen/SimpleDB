#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type_def.h"

#define MAX_INPUT_LEN 1022

static char expr_input[MAX_INPUT_LEN + 2];

char start_parse(char *);


int main(int argc, char **argv) {
    int i, tmp;
    if (argc < 2) {
        return start_parse(NULL);
    } else {
        strcpy(expr_input, argv[1]);
        // fgets(expr_input, MAX_INPUT_LEN, stdin);

        if (start_parse(expr_input))
            return 1;
    }
    return 0;
}
