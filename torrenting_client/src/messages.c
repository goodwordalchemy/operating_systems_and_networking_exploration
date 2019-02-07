#include <errno.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "logging_utils.h"
#include "messages.h"
#include "pieces.h"
#include "socket_helpers.h"
#include "state.h"

#define DEBUG 1
#define N_INTEGER_BYTES 4

#if defined(DEBUG) && DEBUG > 0
 #define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
 #define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
#endif

void print_my_status(){
    int i, bitfield, cur, n_pieces, downloaded, left, uploaded;
    char *headers[] = {"Bitfield", "Downloaded", "Uploaded", "Left"};
    int n_headers = 4;
    
    printf("My status:\n");
    printf("\t");
    for (i = 0; i < n_headers; i++){
        print_str_cell(headers[i]);
    }
    printf("\n");

    print_horizontal_line(n_headers * (3 +COLUMN_WIDTH));

    n_pieces = localstate.n_pieces;
    bitfield = what_is_my_bitfield() >> how_many_shift_bits_in_my_bitfield();

    printf("\t");
    downloaded = 0;
    for (i = 0; i < n_pieces; i++){
        cur = bitfield >> (n_pieces - 1 - i) & 1;
        printf("%d", cur);
        if (cur)
            downloaded++;
    }
    printf("%*s | ", COLUMN_WIDTH - n_pieces, "");

    uploaded = 0; // Nobody has uploaded anything to anyone yet!
    left = n_pieces - downloaded;

    print_int_cell(downloaded);
    print_int_cell(uploaded); 
    print_int_cell(left);
    printf("\n");
    
    print_horizontal_line(n_headers * (3 +COLUMN_WIDTH));

}

unsigned long get_timestamp(){
    return (unsigned long)time(NULL);
}

void encode_int_as_char(int num, char *buf, int length){
    int i;
    for (i = 0; i < length; i++){
        buf[i] = (char)(num >> ((8 * (length - i - 1)) & 0xFF));
    }
}

int decode_int_from_char(char *enc, int length){
    int i, num, cur;

    num = 0;
    for (i = 0; i < length; i++){
        cur = (unsigned char) enc[i] << (8 * (length - i - 1));
        num += (int) cur;
    }

    return num;
}

int send_peer_message(int sockfd, msg_t *msg){
    char *buf;
    int full_length, nbytes;

    full_length = msg->length + N_INTEGER_BYTES + 1;

    buf = malloc(sizeof(char) * full_length);

    encode_int_as_char(msg->length, buf, N_INTEGER_BYTES);

    buf[N_INTEGER_BYTES] = msg->type;

    memcpy(buf + N_INTEGER_BYTES + 1, msg->payload, msg->length - 1);

    DEBUG_PRINT("sending message of type %d\n", msg->type);

    nbytes = send_on_socket(sockfd, buf, full_length);
    
    free(buf);

    return nbytes;
}


int how_many_shift_bits_in_my_bitfield(){
    return 8 - (localstate.n_pieces % 8);
}

int how_many_bytes_in_my_bitfield(){
    int n_shift_bits = how_many_shift_bits_in_my_bitfield();
    int n_piece_bits = localstate.n_pieces + n_shift_bits;
    int n_bitfield_bits = n_shift_bits + n_piece_bits;
    int n_bitfield_bytes = n_bitfield_bits / 8;

    return n_bitfield_bytes;
}

int what_is_my_bitfield(){
    int i, acc;
    int n_shift_bits = how_many_shift_bits_in_my_bitfield();

    acc = 0;
    for (i = 0; i < localstate.n_pieces; i++){
        if (does_piece_exist(localstate.piece_hash_digests[i]))
            acc += (1 << (localstate.n_pieces - 1 - i));
    }
    acc = acc << n_shift_bits;

    return acc;
}

int send_bitfield_message(int sockfd){
    int bitfield;
    int n_bitfield_bytes = how_many_bytes_in_my_bitfield();
    char bf_buf[n_bitfield_bytes];
    msg_t msg;
    

    // create encoded bitfield
    bitfield = what_is_my_bitfield();
    encode_int_as_char(bitfield, bf_buf, n_bitfield_bytes);

    // create message struct
    msg.length = n_bitfield_bytes + 1;
    msg.type = BITFIELD;
    msg.payload = bf_buf;

    // send message struct
    return send_peer_message(sockfd, &msg);
}

void print_peer_bitfields(){
    int i;
    peer_t *p;
    char *headers[] = {"Sockfd", "Status", "Bitfield", "Down/s", "Up/s"};
    int n_headers = 5;

    printf("Peer bitfields:\n");

    printf("\t");
    for (i = 0; i < n_headers; i++){
        print_str_cell(headers[i]);
    }
    printf("\n");

    print_horizontal_line(n_headers * (3 +COLUMN_WIDTH));

    printf("\t");
    for (i = 0; i<MAX_SOCKFD; i++){
        if ((p = localstate.peers[i]) == NULL)
            continue;

        print_int_cell(i);
        print_str_cell("0101");
        print_bitfield_cell(p->bitfield >> how_many_shift_bits_in_my_bitfield());
        print_int_cell(0);
        print_int_cell(0);
    }
    printf("\n");

    print_horizontal_line(n_headers * (3 +COLUMN_WIDTH));
}

void add_peer(int sockfd, int bitfield){
    peer_t *p;
    
    if ((p = (peer_t *)malloc(sizeof(peer_t))) == NULL)
        perror("malloc");

    p->bitfield = bitfield;
    p->last_contact = 0;
    p->requested_piece = -1;

    localstate.peers[sockfd] = p;

    printf("Added a new peer.\n");
};

int handle_bitfield_message(int sockfd, msg_t *msg){
    // If bitfield message validates, then it will set the bitfield field on the peer struct at the sockfd.
    int bitfield;

    // For receiving messsage
    int n_bitfield_bytes = how_many_bytes_in_my_bitfield();;

    // for parsing message
    int n_shift_bits = 8 - (localstate.n_pieces % 8);

    if (msg->length != n_bitfield_bytes + 1){
        fprintf(stderr, "Expected length prefix %d.  Instead got %d\n", n_bitfield_bytes + 1, msg->length);
        return -1;
    }

    if (msg->type != BITFIELD){
        fprintf(stderr, "expected message type of BITFIELD=5 from peer. Instead got %d\n", msg->type);
        return -1;
    }

    bitfield = decode_int_from_char(msg->payload, n_bitfield_bytes);

    if ((bitfield & ((int) pow((double) 2, n_shift_bits) - 1)) > 0){
        fprintf(stderr, "trailing zeros were set in peers bitfield\n");
        return -1;
    }

    add_peer(sockfd, bitfield);

    return 0;
}

int choose_a_piece_to_request(){
    int bitfield, i, power;

    bitfield = what_is_my_bitfield() >> how_many_shift_bits_in_my_bitfield();

    for (i = 0; i < localstate.n_pieces; i++){
        power = (int)pow((double)2, localstate.n_pieces - 1 - i);
        if (power & bitfield)
            continue;
        return i;
    }

    return -1;
}

int choose_a_peer_to_request_from(){
    int i;
    peer_t *p;
    for (i = 0; i < localstate.max_sockfd; i++){
        if ((p = localstate.peers[i]) == NULL){
            continue;
        }
        if (p->last_contact == 0 || p->requested_piece == -1){
            return i;
        }
    }
    return -1;
}

int send_request_message(int sockfd, int piece){
    msg_t msg;
    char data[12];

    encode_int_as_char(piece, data, 4);
    encode_int_as_char(0, data + 4, 4);
    encode_int_as_char(localstate.piece_length, data + 8, 4);

    msg.length = 13;
    msg.type = REQUEST;
    msg.payload = data;

    if (send_peer_message(sockfd, &msg) <= 0){
        fprintf(stderr, "could not send piece request.");
        return 0;
    }

    localstate.peers[sockfd]->last_contact = get_timestamp();
    localstate.peers[sockfd]->requested_piece = piece;

    DEBUG_PRINT("REQUESTing piece %d from sockfd %d\n", piece, sockfd);

    return 1;
}


void send_request_messages(){
    int rpeer, piece;

    while ((piece = choose_a_piece_to_request()) != -1){
        if ((rpeer = choose_a_peer_to_request_from()) == -1)
            break;
        send_request_message(rpeer, piece);
    }
}

int send_piece_message(int sockfd, int piece_idx){
    msg_t msg;
    FILE *f;
    int begin, piece_length, nbytes;
    char *piece_hash_digest, *payload_buf;
    char piece_filename[FILENAME_WITH_EXT_BUFLEN];
    
    DEBUG_PRINT("sending a PIECE message: %d\n", piece_idx);

    msg.type = PIECE;

    piece_hash_digest = localstate.piece_hash_digests[piece_idx];
    get_filename_with_extension(piece_hash_digest, piece_filename);

    if ((f = fopen(piece_filename, "r")) == NULL)
        return -1;

    if (piece_idx == localstate.n_pieces-1)
        piece_length = localstate.last_piece_size;
    else
        piece_length = localstate.piece_length;

    msg.length = 1 + 4 + 4 + piece_length; // msg_id + index + begin + block

    if ((payload_buf = malloc((msg.length) * sizeof(char))) == NULL){
        perror("malloc");
        return -1;
    }

    encode_int_as_char(piece_idx, payload_buf, N_INTEGER_BYTES);

    begin = 0; // Maybe later I'll implement block sizes other than piece size...
    encode_int_as_char(begin, payload_buf + 4, N_INTEGER_BYTES);

    if (fread(payload_buf+8, 1, piece_length, f) == 0){
        perror("malloc");
        return -1;
    }

    msg.payload = payload_buf;

    nbytes = send_peer_message(sockfd, &msg);

    free(payload_buf);

    return nbytes;
}

int handle_request_message(int sockfd, msg_t *msg){
    int index, begin, length;

    if (msg->length != 13){
        fprintf(stderr, "Request message did not specify correct length. Should be 13, but it was %d.\n", msg->length);
        return -1;
    }

    index = decode_int_from_char(msg->payload, 4);
    begin = decode_int_from_char(msg->payload + 4, 4);
    length = decode_int_from_char(msg->payload + 8, 4);
    
    if (begin != 0){
        fprintf(stderr, "Have not implemented sending blocks that aren't the first in their piece\n");
        return -1;
    }

    if (length == localstate.piece_length);
    else if (length == localstate.last_piece_size);
    else {
        fprintf(stderr, "Have not implemented sending blocks not equal to the piece size yet\n");
        return -1;
    }

    if (send_piece_message(sockfd, index) <= 0){
        fprintf(stderr, "There was an error fulfilling a piece request\n");
        return -1;
    }

    return 0;
}
void send_have_messages(int index){
    msg_t msg;
    int i;
    char payload[4];

    encode_int_as_char(index, payload, N_INTEGER_BYTES);

    msg.length = 5;
    msg.type = HAVE;
    msg.payload = payload;

    for (i = 0; i < localstate.max_sockfd; i++){
        if (localstate.peers[i] == NULL)
            continue;
        send_peer_message(i, &msg);
    }
}

int handle_piece_message(int sockfd, msg_t *msg){
    FILE *f;
    int index, begin, expected_length, piece_length, next_piece;
    char *piece_contents, *cur_hash_digest;
    char piece_filename[FILENAME_WITH_EXT_BUFLEN];

    DEBUG_PRINT("got a PIECE message! index: %d, begin %d, length: %d\n", 
            decode_int_from_char(msg->payload, 4),
            decode_int_from_char(msg->payload + 4, 4),
            msg->length);

    index = decode_int_from_char(msg->payload, 4);
    begin = decode_int_from_char(msg->payload + 4, 4);
    piece_length = decode_int_from_char(msg->payload + 8, 4);

    if (index != localstate.peers[sockfd]->requested_piece){
        fprintf(stderr, "Index of send piece wrong.  Requested %d.  Got %d\n",
                localstate.peers[sockfd]->requested_piece, index);
        return -1;
    }
    
    if (begin != 0){
        fprintf(stderr, "Have not implemented sending blocks that aren't the first in their piece\n");
        return -1;
    }

    if (index == localstate.n_pieces-1)
        piece_length = localstate.last_piece_size;
    else 
        piece_length = localstate.piece_length;


    expected_length = piece_length + 9;
    if (msg->length != expected_length){
        fprintf(stderr, "Piece message was not the correct length.  Should be %d, was %d\n", expected_length, msg->length);
        return -1;
    }

    cur_hash_digest = localstate.piece_hash_digests[index];
    piece_contents = msg->payload + 8;

    if (validate_piece(cur_hash_digest, piece_contents, piece_length) == -1){
        fprintf(stderr, "Piece received did not validate against it's hash.\n");
        return -1;
    }

    get_filename_with_extension(cur_hash_digest, piece_filename);
    if ((f = fopen(piece_filename, "w")) == NULL){
        perror("fopen");
        return -1;
    }

    if (fwrite(piece_contents, 1, piece_length, f) <= 0){
        perror("fwrite");
        return -1;
    }

    fclose(f);

    // Note, bitfield is automatically updated, because piece hash file exists.
    
    // flush request messages before broadcasting have in order to prevent request message getting lost in rapid succession of
    // have / request message on same buffer.
    localstate.peers[sockfd]->requested_piece = -1;
    send_request_messages();

    print_my_status();

    send_have_messages(index);

    return 0;
}

int handle_have_message(int sockfd, msg_t *msg){
    int index;

    if (msg->length != 5){
        fprintf(stderr, "Have message was not correct length.\n");
        return -1;
    }

    index = decode_int_from_char(msg->payload, N_INTEGER_BYTES);

    printf("received HAVE message on sockfd: %d for piece %d\n", sockfd, index);
    localstate.peers[sockfd]->bitfield |= (int)pow((double) 2, localstate.n_pieces - 1 - index) << how_many_shift_bits_in_my_bitfield();

    return 0;
}

int receive_peer_message(int sockfd){
    msg_t msg;
    int nbytes, msg_type, i;
    char length_buffer[N_INTEGER_BYTES];

    // longest possible length is in a piece message.
    // (length prefix) (msg type) (index) (begin) (piece length) + null-termination (received) + null-termination (in receive function);
    int longest_possible_length = 4 + 1 + 4 + 4 + localstate.piece_length + 2; 
    char receive_buffer[longest_possible_length];

    if ((nbytes = receive_on_socket(sockfd, receive_buffer, longest_possible_length)) <= 0)
        return -1;

    // DEBUGGING
    /* printf("These are the first 15 bytes of the message received:\n"); */
    /* for (i = 0; i < 15; i++) */
    /*     printf("%02x", receive_buffer[i]); */
    /* printf("\n"); */
    /////////////

    for (i = 0; i < N_INTEGER_BYTES; i++)
        length_buffer[i] = receive_buffer[i];
    msg.length = decode_int_from_char(length_buffer, N_INTEGER_BYTES);

    msg_type = receive_buffer[N_INTEGER_BYTES];
    msg.type = msg_type;

    DEBUG_PRINT("received msg.type: %d\n", msg.type);

    if (msg.type != BITFIELD && localstate.peers[sockfd] == NULL)
        return -1;

    msg.payload = receive_buffer + N_INTEGER_BYTES + 1;

    switch(msg.type){
        case (CHOKE):
            break;
        case (UNCHOKE):
            break;
        case (INTERESTED):
            break;
        case (NOT_INTERESTED):
            break;
        case (HAVE):
            if (handle_have_message(sockfd, &msg) == -1)
                return -1;
            break;
        case (BITFIELD):
            if (handle_bitfield_message(sockfd, &msg) == -1)
                return -1;
            break;
        case (REQUEST):
            if (handle_request_message(sockfd, &msg) == -1)
                return -1;
            break;
        case (PIECE):
            if (handle_piece_message(sockfd, &msg) == -1)
                return -1;
            break;
        case (CANCEL):
            break;
        default:
            fprintf(stderr, "Unknown message type: %d\n", msg.type);
            return -1;
    }

    return nbytes;
}
