#include <errno.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

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

int receive_peer_message(int sockfd, char *buf, int length){
    int nbytes;

    nbytes = receive_on_socket(sockfd, buf, length);

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


int receive_bitfield_message(int sockfd){
    int i;

    // For receiving messsage
    int nbytes;
    int n_bitfield_bytes = how_many_bytes_in_my_bitfield();;
    int expected_msg_length = LENGTH_PREFIX_BITS + 1 + n_bitfield_bytes;
    char buf[expected_msg_length + 1];

    // for parsing message
    int n_shift_bits = 8 - (localstate.n_pieces % 8);
    int bitfield, length;
    char msg_type;
    char length_buf[5];
    char bf_buf[n_bitfield_bytes+1];

    if ((nbytes = receive_peer_message(sockfd, buf, expected_msg_length + 1)) <= 0)
        return -1;

    // For debugging
    /* printf("received bitfield message from peer length=(%d)\n", nbytes); */
    /* for (i = 0; i < nbytes; i++) */
    /*     printf("%02x", (unsigned char) buf[i]); */
    /* printf("\n"); */

    for (i = 0; i < LENGTH_PREFIX_BITS; i++)
        length_buf[i] = buf[i];

    msg_type = buf[LENGTH_PREFIX_BITS];

    for (i = 0; i < n_bitfield_bytes; i++)
        bf_buf[i] = buf[i + LENGTH_PREFIX_BITS + 1];

    length = decode_int_from_char(length_buf, LENGTH_PREFIX_BITS);
    if (length != n_bitfield_bytes + 1){
        fprintf(stderr, "Expected length prefix %d.  Instead got %d\n", n_bitfield_bytes + 1, length);
        return -1;
    }

    if (msg_type != BITFIELD){
        fprintf(stderr, "expected message type of BITFIELD=5 from peer. Instead got %d\n", msg_type);
        return -1;
    }

    bitfield = decode_int_from_char(bf_buf, n_bitfield_bytes);

    if ((bitfield & ((int) pow((double) 2, n_shift_bits) - 1)) > 0)
        fprintf(stderr, "trailing zeros were set in peers bitfield\n");

    return bitfield;
}


int choose_a_piece_to_request(){
    int bitfield, i;

    bitfield = what_is_my_bitfield() >> how_many_shift_bits_in_my_bitfield();

    for (i = 0; i < localstate.n_pieces; i++)
        if ((int)pow((double)2, i) & bitfield)
            break;


    return i;
}

int send_request_message(int sockfd){
    int requesting_piece;
    msg_t msg;
    char data[12];
    
    requesting_piece = choose_a_piece_to_request();

    encode_int_as_char(requesting_piece, data, 4);
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
    localstate.peers[sockfd]->requested_piece = requesting_piece;

    return 1;
}
