#include <stdio.h>
#include <string.h>
#include <time.h>

#include "logging_utils.h"
#include "state.h"

unsigned long get_epoch_time(){
    return (unsigned long)time(NULL);
}

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

void print_horizontal_line(int width){
    int i;
    printf("\t");
    for (i = 0; i < width; i++)
        printf("%s", "-");
    printf("\n");
}
