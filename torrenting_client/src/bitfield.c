#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "encoding_utils.h"
#include "pieces.h"
#include "state.h"

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
    int acc, i;
    acc = 0;
    for (i = 0; i < localstate.n_pieces; i++){
        if (does_piece_exist(localstate.piece_hash_digests[i]))
            acc += (1 << (localstate.n_pieces - 1 - i));
    }

    return acc;
}

char *calculate_my_bitfield(){
    int i, j, p, acc, nbytes, shift_bits, until;
    char *bitfield;

    nbytes = how_many_bytes_in_my_bitfield();
    shift_bits = how_many_shift_bits_in_my_bitfield();

    bitfield = malloc(sizeof(char) * nbytes);

    acc = 0;
    until = localstate.n_pieces - 1 - (8 - shift_bits);
    printf("until: %d\n", until);
    printf("shift bits: %d\n", shift_bits);

    for (i=7, p = localstate.n_pieces - 1; p > until; p--, i--){
        if (does_piece_exist(localstate.piece_hash_digests[p])){
            acc += (1 << (7 - i));
            printf("i=%d, acc=%d\n", i, acc);
        }
    }

    acc = acc << shift_bits;
    printf("acc for byte %d: %d\n", nbytes - 1, acc);
        
    encode_int_as_char(acc, bitfield + nbytes - 1, 1);

    printf("stored last byte: %d\n", decode_int_from_char(bitfield+nbytes - 1, 1));

    if (nbytes > 1){
        for (j = 0; j < nbytes - 1; j++){
            acc = 0;
            for (i = 0; i < 8; i++){
                if (does_piece_exist(localstate.piece_hash_digests[i+(j*8)]))
                    acc += (1 << (7-i));
                else
                    printf("piece does not exist at idx: %d\n", i+(j*8));
                printf("i=%d, acc=%d\n", i, acc);
            }
            encode_int_as_char(acc, bitfield + j, 1);
        }
    }

    for (i = 0; i < localstate.n_pieces; i++)
        if (!does_piece_exist(localstate.piece_hash_digests[i]))
            printf("piece idx=%d, digest=%s\n", i, localstate.piece_hash_digests[i]);

    return bitfield;
}

int bitfield_has_piece(char *bitfield, int index){
    int byte_num, byte_val, bit_idx, ret;

    byte_num = index / 8;
    bit_idx = index % 8;

    byte_val = decode_int_from_char(bitfield+byte_num, 1);

    ret = byte_val & (int) pow((double) 2, 7 - bit_idx);

    printf("\nret for idx %d: %d, byte=%d----->", index, ret, byte_val);

    ret = ret >> (7 - bit_idx);

    return ret;
}

void print_bitfield(char *bitfield){
    int i;
    for (i = 0; i < localstate.n_pieces; i++){
        if (bitfield_has_piece(bitfield, i))
            printf("1");
        else
            printf("0");
    }

}

void store_my_bitfield(){
    localstate.bitfield = calculate_my_bitfield();

    //////
    print_bitfield(localstate.bitfield);
    exit(0);
    //////
}


int peer_has_piece(int sockfd, int index){
    int bitfield;

    bitfield = localstate.peers[sockfd]->bitfield;

    /* return bitfield_has_piece(bitfield, index); */
    return 0;
}

int i_have_piece(int index){
    int bitfield;

    bitfield = what_is_my_bitfield();

    /* return bitfield_has_piece(bitfield, index); */
    return 0;
}

void add_piece_to_bitfield(char *bitfield, int index){
}
