#include <math.h>

#include "pieces.h"
#include "state.h"

int what_is_my_bitfield(){
    int i, acc;

    acc = 0;
    for (i = 0; i < localstate.n_pieces; i++){
        if (does_piece_exist(localstate.piece_hash_digests[i]))
            acc += (1 << (localstate.n_pieces - 1 - i));
    }

    return acc;
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

int bitfield_has_piece(int bitfield, int index){
    int power;

    power = (int)pow((double)2, localstate.n_pieces - 1 - index);

    if (power & bitfield)
        return 1;
    return 0;
}

int peer_has_piece(int sockfd, int index){
    int bitfield;

    bitfield = localstate.peers[sockfd]->bitfield;

    return bitfield_has_piece(bitfield, index);
}

int i_have_piece(int index){
    int bitfield;

    bitfield = what_is_my_bitfield();

    return bitfield_has_piece(bitfield, index);
}

