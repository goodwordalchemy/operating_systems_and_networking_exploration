#define MAX_SEQ_NUM 2147483647
#define HEADER_SIZE 8
#define CHECKSUM_SIZE 2

void int_to_char(int n, char *array, int nbytes);

int char_to_int(char *array, int nbytes);

void calculate_checksum(char *message, int length, char *checksum, int cslength);

int verify_checksum(char *message, int length, char *checksum, int cslength);

void add_checksum_at_index(char *data, int index);

int check_packet_not_corrupted(char *data);
