#include <stdio.h>
#include <string.h>

#include "logging_utils.h"

void print_str_cell(char *value){
    int indent;

    indent = COLUMN_WIDTH - strlen(value);
    printf("%s%*s | ", value, indent, "");
}

void print_int_cell(int value){
    char buf[COLUMN_WIDTH];

    snprintf(buf, COLUMN_WIDTH, "%d", value);

    print_str_cell(buf);
}

