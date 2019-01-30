#define MAX_SEQ_NUM 2147483647
#define HEADER_SIZE 5

void int_to_char(int n, char *array, int nbytes);

int char_to_int(char *array, int nbytes);

void calculate_checksum(char *message, int length, char *checksum, int cslength);

int verify_checksum(char *message, int length, char *checksum, int cslength);
