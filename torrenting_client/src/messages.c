#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "messages.h"
#include "socket_helpers.h"
#include "state.h"

#define LENGTH_PREFIX_BITS 4

void encode_int_as_char(int num, char *buf, int length){
    int i;
    for (i = 0; i < length; i++){
        buf[i] = num >> (8 * (length - i - 1));
    }
}

int decode_int_from_char(char *enc, int length){
    int i, num;

    num = 0;
    for (i = 0; i < length; i++){
        num += enc[i] << (8 * (length - i - 1));
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

    buf[full_length] = 0;

    nbytes = send_on_socket(sockfd, buf, full_length);

    free(buf);

    return nbytes;
}

int receive_peer_message(int sockfd, char *buf){
    int nbytes;

    nbytes = receive_on_socket(sockfd, buf);


    return nbytes;
}

int _does_file_exist(char *filename){
    struct stat buf;
    if (stat(filename, &buf) == -1){
        if (errno != ENOENT)
            perror("stat");
        return 0;
    }
    return 1;
}

int send_bitfield_message(int sockfd){
    int i, acc;
    char *piece_hash;
    msg_t msg;
    
    int n_piece_bits = localstate.n_pieces / 8 + 1;
    char bf_buf[n_piece_bits];

    char *filename = localstate.file_name;
    char temp_filename_buffer[SHA_DIGEST_LENGTH + strlen(filename) + 1];

    // create encoded bitfield
    memcpy(temp_filename_buffer, filename, strlen(filename));
    acc = 0;
    for (i = 0; i < localstate.n_pieces; i++){
        piece_hash = (char*) localstate.piece_hashes[i];
        memcpy(temp_filename_buffer + strlen(filename), piece_hash, SHA_DIGEST_LENGTH + 1);
        if (_does_file_exist(temp_filename_buffer))
            acc += 1 << i;
    }
    acc = acc << (i % 8);
    encode_int_as_char(acc, bf_buf, n_piece_bits);

    // create message struct
    msg.length = n_piece_bits;
    msg.type = BITFIELD;
    msg.payload = bf_buf;

    // send message struct
    return send_peer_message(sockfd, &msg);
}


int receive_bitfield_message(int sockfd){
    int nbytes;
    int expected_msg_length = LENGTH_PREFIX_BITS + 1 + (localstate.n_pieces / 8 + 1);
    char buf[expected_msg_length];

    nbytes = receive_peer_message(sockfd, buf);

    printf("received bitfield message from peer: %s\n", buf);

    return nbytes;
}

