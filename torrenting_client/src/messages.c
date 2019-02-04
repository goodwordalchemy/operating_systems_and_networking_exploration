#include <errno.h>
#include <math.h>
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
        if (_does_file_exist(temp_filename_buffer))
            acc += (1 << (localstate.n_pieces - 1 - i));
    }
    acc = acc << n_shift_bits;
    encode_int_as_char(acc, bf_buf, n_bitfield_bytes);

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
    int n_bitfield_bytes = (localstate.n_pieces / 8 + 1);
    int expected_msg_length = LENGTH_PREFIX_BITS + 1 + n_bitfield_bytes;
    char buf[expected_msg_length + 1];

    // for parsing message
    int n_shift_bits = 8 - (localstate.n_pieces % 8);
    int bitfield, length;
    char msg_type;
    char fmt[12];
    char length_buf[5];
    char bf_buf[n_bitfield_bytes+1];

    if ((nbytes = receive_peer_message(sockfd, buf, expected_msg_length + 1)) <= 0)
        return -1;

    printf("received bitfield message from peer length=(%d)\n", nbytes);
    for (i = 0; i < nbytes; i++)
        printf("%02x", (unsigned char) buf[i]);
    printf("\n");

    for (i = 0; i < LENGTH_PREFIX_BITS; i++)
        length_buf[i] = buf[i];

    msg_type = buf[LENGTH_PREFIX_BITS];

    for (i = 0; i < n_bitfield_bytes; i++)
        bf_buf[i] = buf[i + LENGTH_PREFIX_BITS + 1];

    printf("DEBUG: length: ");
    for (i = 0; i < LENGTH_PREFIX_BITS; i++)
        printf("%02x", (unsigned char) length_buf[i]);
    printf("\n");

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

    printf("DEBUG: bitfield message from peer all good: %d\n", bitfield);
    return bitfield;
}

