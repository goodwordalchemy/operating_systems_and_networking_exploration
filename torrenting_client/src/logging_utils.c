#include <stdio.h>
#include <string.h>

#include "logging_utils.h"
#include "state.h"

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

void print_bitfield_cell(int bitfield){
    int i, left, cur, n_pieces;

    left = COLUMN_WIDTH;
    n_pieces = localstate.n_pieces;

    for (i = 0; i < n_pieces; i++){
        cur = bitfield >> (n_pieces - 1 - i) & 1;
        printf("%d", cur);
    }
    printf("%*s | ", COLUMN_WIDTH - n_pieces, "");

}


