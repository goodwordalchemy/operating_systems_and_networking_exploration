#include <errno.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "logging_utils.h"
#include "filestring.h"
#include "messages.h"
#include "socket_helpers.h"
#include "state.h"

#define LENGTH_PREFIX_BITS 4

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

    full_length = msg->length + LENGTH_PREFIX_BITS + 1;

    buf = malloc(sizeof(char) * (full_length + 1));

    encode_int_as_char(msg->length, buf, LENGTH_PREFIX_BITS);

    buf[LENGTH_PREFIX_BITS] = msg->type;

    memcpy(buf + LENGTH_PREFIX_BITS + 1, msg->payload, msg->length);

    buf[full_length+1] = 0;

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
        if (does_file_exist(localstate.piece_hash_digests[i]))
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
    
    p = malloc(sizeof(p));
    p->bitfield = bitfield;
    p->requested_piece = -1;
    p->last_contact = -1;

    localstate.peers[sockfd] = p;

    printf("Added a new peer.\n");
    print_peer_bitfields();
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

    return 1;
}

int choose_a_piece_to_request(){
    int bitfield, i, power;

    bitfield = what_is_my_bitfield() >> how_many_shift_bits_in_my_bitfield();

    for (i = 0; i < localstate.n_pieces; i++){
        power = (int)pow((double)2, i);
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
        if ((p = localstate.peers[i]) == NULL)
            continue;
        if (p->requested_piece == -1)
            return i;
    }
    return -1;
}

void send_piece_requests(){
    int rpeer, piece;

    while ((piece = choose_a_piece_to_request()) != -1){
        if ((rpeer = choose_a_peer_to_request_from()) == -1)
            break;
        send_request_message(rpeer, piece);
    }
}

int handle_piece_request(int sockfd, msg_t *msg){
    int index, begin, length;

    if (msg->length != 13){
        fprintf(stderr, "Request message did not specify correct length.\n");
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

    printf("Received request for piece at index %d.  Need to fulfill it", index);

    return 0;
}

int receive_peer_message(int sockfd){
    msg_t msg;
    int nbytes, msg_type, i;
    char length_buffer[LENGTH_PREFIX_BITS];

    int longest_possible_length = 4 + 1 + 4 + 4 + localstate.piece_length; 
    char receive_buffer[longest_possible_length + 1];

    if ((nbytes = receive_on_socket(sockfd, receive_buffer, longest_possible_length)) <= 0)
        return -1;

    // DEBUGGING
    /* printf("These are the first 15 bytes of the message received:\n"); */
    /* for (i = 0; i < 15; i++) */
    /*     printf("%02x", receive_buffer[i]); */
    /* printf("\n"); */
    /////////////

    for (i = 0; i < LENGTH_PREFIX_BITS; i++)
        length_buffer[i] = receive_buffer[i];
    msg.length = decode_int_from_char(length_buffer, LENGTH_PREFIX_BITS);

    msg_type = receive_buffer[LENGTH_PREFIX_BITS];
    msg.type = msg_type;

    printf("DEBUG: msg.type: %d\n", msg.type);

    if (msg.type != BITFIELD && localstate.peers[sockfd] == NULL)
        return -1;

    msg.payload = receive_buffer + LENGTH_PREFIX_BITS + 1;

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
            break;
        case (BITFIELD):
            if (handle_bitfield_message(sockfd, &msg) == -1)
                return -1;
        case (REQUEST):
            if (handle_piece_request(sockfd, &msg) == -1)
                return -1;
        case (PIECE):
            break;
        case (CANCEL):
            break;
        default:
            fprintf(stderr, "Unknown message type: %d\n", msg.type);
            return -1;
    }

    return nbytes;
}
