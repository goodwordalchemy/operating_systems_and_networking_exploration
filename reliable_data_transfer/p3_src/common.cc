#include "common.h"

void int_to_char4(int n, char *array){
    int i;
    for (i = 0; i < 4; i++){
        array[i] = ((n >> ((3-i) * 8)) & 0xFF) - 128;
    }


}

int char4_to_int(char *array){
    int i, ret, cur;
    ret = 0;
    for (i = 0; i < 4; i++){
        cur = (array[i] + 128) << ((3 - i) * 8);
        ret += (int) cur;
    }

    return ret;
}
