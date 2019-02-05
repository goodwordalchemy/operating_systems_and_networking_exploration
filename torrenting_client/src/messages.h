typedef struct {
    char *bitfield;
    size_t size;
} bitfield_t;

typedef struct {
    int index;
    int begin;
    int length;
} request_t;

typedef struct {
    int index;
    int begin;
    char piece[0];
} piece_t;

typedef enum msg_id {
    CHOKE,
    UNCHOKE,
    INTERESTED,
    NOT_INTERESTED,
    HAVE,
    BITFIELD,
    REQUEST,
    PIECE,
    CANCEL
} msg_id_t;

typedef struct msg {
    int length;
    msg_id_t type;

    char *payload;
} msg_t;

int send_bitfield_message(int sockfd);
int receive_bitfield_message(int sockid);


int what_is_my_bitfield();
int how_many_shift_bits_in_my_bitfield();
