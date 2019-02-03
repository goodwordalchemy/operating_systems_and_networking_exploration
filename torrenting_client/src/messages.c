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
        buf[i] = num >> (char)((8 * (length - i - 1)) & 0xFF);
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

    printf("DEBUG: encoded length: %s\n", buf);

    buf[LENGTH_PREFIX_BITS] = msg->type;

    memcpy(buf + LENGTH_PREFIX_BITS + 1, msg->payload, msg->length);

    buf[full_length+1] = 0;

    nbytes = send_on_socket(sockfd, buf, full_length);
    
    printf("DEBUG: msg length before: %d, after: %d\n", msg->length, decode_int_from_char(buf, 4));
    printf("DEBUG: msg type before: %d, msg type after: %d\n", msg->type, buf[LENGTH_PREFIX_BITS]);
    printf("DEBUG: msg payload before: %.*s, after: %.*s\n", msg->length, msg->payload, msg->length, buf+LENGTH_PREFIX_BITS+1);
    printf("DEBUG: msg payload (1st byte) before: %d, after: %d\n", msg->payload[0], buf[LENGTH_PREFIX_BITS+1]);
    printf("DEBUG: sizeof msg: %lu\n", sizeof(buf));

    free(buf);

    return nbytes;
}

int receive_peer_message(int sockfd, char *buf, int length){
    int nbytes;

    nbytes = receive_on_socket(sockfd, buf, length);

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
    
    int n_shift_bits = 8 - (localstate.n_pieces % 8);
    int n_piece_bits = localstate.n_pieces + n_shift_bits;
    int n_bitfield_bits = n_shift_bits + n_piece_bits;
    int n_bitfield_bytes = n_bitfield_bits / 8;
    char bf_buf[n_bitfield_bytes];

    char *filename = localstate.file_name;
    char temp_filename_buffer[SHA_DIGEST_LENGTH + strlen(filename) + 1];

    // create encoded bitfield
    memcpy(temp_filename_buffer, filename, strlen(filename));
    acc = 0;
    for (i = 0; i < localstate.n_pieces; i++){
        piece_hash = (char*) localstate.piece_hashes[i];
        memcpy(temp_filename_buffer + strlen(filename), piece_hash, SHA_DIGEST_LENGTH + 1);
        if (_does_file_exist(temp_filename_buffer) || i == localstate.n_pieces - 1 || i == 0)
            acc += (1 << (localstate.n_pieces - 1 - i));
        printf("DEBUG: i=%d, acc=%d\n", i, acc);
    }
    printf("DEBUG: acc %d\n", acc);
    printf("DEBUG: npieces: %d\n", localstate.n_pieces);
    printf("DEBUG: npiece bits: %d\n", n_piece_bits);
    printf("DEBUG: shifting %d bits\n", (n_shift_bits));
    acc = acc << n_shift_bits;
    printf("DEBUG: acc shifted %d\n", acc);
    encode_int_as_char(acc, bf_buf, n_bitfield_bytes);
    printf("DEBUG: this should equal acc shifted: %d\n", decode_int_from_char(bf_buf, n_bitfield_bytes));

    // create message struct
    msg.length = n_bitfield_bytes;
    msg.type = BITFIELD;
    msg.payload = bf_buf;

    // send message struct
    return send_peer_message(sockfd, &msg);
}


int receive_bitfield_message(int sockfd){
    int nbytes;
    int expected_msg_length = LENGTH_PREFIX_BITS + 1 + (localstate.n_pieces / 8 + 1);
    char buf[expected_msg_length + 1];

    nbytes = receive_peer_message(sockfd, buf, expected_msg_length + 1);

    printf("received bitfield message from peer length=(%d)\n", nbytes);
    int i;
    for (i = 0; i < nbytes; i++)
        printf("%02x", buf[i]);
    printf("\n");

    return nbytes;
}

