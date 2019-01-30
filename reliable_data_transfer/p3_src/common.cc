#include <string.h>

#include "common.h"
#include "rdt_struct.h"

void int_to_char(int n, char *array, int nbytes){
    int i;
    for (i = 0; i < nbytes; i++){
        array[i] = ((n >> ((nbytes-1-i) * 8)) & 0xFF) - 128;
    }
}

int char_to_int(char *array, int nbytes){
    int i, ret, cur;
    ret = 0;
    for (i = 0; i < nbytes; i++){
        cur = (array[i] + 128) << ((nbytes-1-i) * 8);
        ret += (int) cur;
    }

    return ret;
}

void calculate_checksum(char *message, int length, char *checksum, int cslength){
    int i; 
    unsigned long sum;
    unsigned short word16;

    sum = 0;
    for (i = 0; i < length; i += 2){
        word16 = ((message[i]<<8)&0xFF00)+(message[i+1]&0xFF);
        sum += (unsigned long) word16;
    }

    /* add the 16 carries from the left. */
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    word16 = ~(unsigned short) sum;

    int_to_char(word16, checksum, cslength);

}

int verify_checksum(char *message, int length, char *checksum, int cslength){
    char check[cslength];
    calculate_checksum(message, length, check, cslength);

    if (char_to_int(check, cslength) == char_to_int(checksum, cslength)){
        return 1;
    }
    return 0;
}

void add_checksum_at_index(char *data, int index){
    char checksum[CHECKSUM_SIZE];

    memset(data+index, 0, CHECKSUM_SIZE + 1);
    calculate_checksum(data, RDT_PKTSIZE, checksum, CHECKSUM_SIZE);
    memcpy(data+index, checksum, CHECKSUM_SIZE);
}

int check_packet_not_corrupted(char *data){
    int res;
    char checksum[CHECKSUM_SIZE];
    char data_copy[RDT_PKTSIZE];

    memcpy(checksum, data+5, CHECKSUM_SIZE);

    memcpy(data_copy, data, RDT_PKTSIZE);
    memset(data_copy+5, 0, CHECKSUM_SIZE + 1);

    res = verify_checksum(data_copy, RDT_PKTSIZE, checksum, CHECKSUM_SIZE);

    return res;
}

