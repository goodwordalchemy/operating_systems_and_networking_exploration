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

char *what_is_my_bitfield(){
    return localstate.bitfield;
}

char *calculate_my_bitfield(){
    int i, j, p, acc, nbytes, shift_bits, until;
    char *bitfield;

    nbytes = how_many_bytes_in_my_bitfield();
    shift_bits = how_many_shift_bits_in_my_bitfield();

    bitfield = malloc(sizeof(char) * nbytes);

    acc = 0;
    until = localstate.n_pieces - 1 - (8 - shift_bits);

    for (i=7, p = localstate.n_pieces - 1; p > until; p--, i--){
        if (does_piece_exist(localstate.piece_hash_digests[p])){
            acc += (1 << (7 - i));
        }
    }

    acc = acc << shift_bits;
        
    encode_int_as_char(acc, bitfield + nbytes - 1, 1);

    if (nbytes > 1){
        for (j = 0; j < nbytes - 1; j++){
            acc = 0;
            for (i = 0; i < 8; i++){
                if (does_piece_exist(localstate.piece_hash_digests[i+(j*8)]))
                    acc += (1 << (7-i));
            }
            encode_int_as_char(acc, bitfield + j, 1);
        }
    }

    return bitfield;
}

int bitfield_has_piece(char *bitfield, int index){
    int byte_num, byte_val, bit_idx, ret;

    byte_num = index / 8;
    bit_idx = index % 8;

    byte_val = decode_int_from_char(bitfield+byte_num, 1);

    ret = byte_val & (int) pow((double) 2, 7 - bit_idx);

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
}


int peer_has_piece(int sockfd, int index){
    int bitfield;

    bitfield = localstate.peers[sockfd]->bitfield;

    /* return bitfield_has_piece(bitfield, index); */
    return 0;
}

int i_have_piece(int index){
    char *bitfield;
    
    bitfield = what_is_my_bitfield();

    return bitfield_has_piece(bitfield, index);
}

void add_piece_to_bitfield(char *bitfield, int index){
}
