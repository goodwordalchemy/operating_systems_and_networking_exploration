#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "filestring.h"
#include "state.h"

int validate_piece(char *piece_hash){
    int i;
    filestring_t *fs;
    int piece_size = localstate.file_size / localstate.n_pieces;
    char buf[piece_size + 1];
    unsigned char hash_buf[SHA_DIGEST_LENGTH+1];

    fs = read_file_to_string(piece_hash);


    SHA1((unsigned char *)fs->data, fs->length, hash_buf); 

    for (i = 0; i < SHA_DIGEST_LENGTH; i++)
        if (piece_hash[i] != (char) hash_buf[i])
            return 0;

    return 1;
}

int create_pieces(){
    int i;
    FILE *target, *tmp;
    int piece_size = localstate.file_size / localstate.n_pieces;
    char buf[piece_size + 1];
    char *cur_piece_hash;

    target = fopen(localstate.file_name, "r");

    for (i = 0; i < localstate.n_pieces; i++){
        cur_piece_hash = (char*) localstate.piece_hashes[i];
        tmp = fopen(cur_piece_hash, "w");

         /* If your file is less than 1000 bytes, fread(a, 1, 1000, stdin) 
          * (read 1000 elements of 1 byte each) will still copy all the bytes 
          * until EOF. On the other hand, the result of fread(a, 1000, 1, stdin) 
          * (read 1 1000-byte element) stored in a is unspecified, because there is 
          * not enough data to finish reading the 'first' (and only) 1000 byte element. */
        if ((fread(buf, 1, piece_size, target)) == 0){
            perror("fread while sharding target");
            fclose(tmp);
            return -1;
        }

        if ((fwrite(buf, 1, piece_size, tmp)) <= 0){
            perror("fwrite while sharding target");
            fclose(tmp);
            return -1;
        }

        if (!validate_piece(cur_piece_hash)){
            fprintf(stderr, "Could not validate piece.\n");
            return -1;
        }
    }

    return 0;
}

void clean_pieces(){
    int i;

    for (i = 0; i < localstate.n_pieces; i++){
        if (unlink((char *)localstate.piece_hashes[i]) < 0)
            perror("unlink");
    }
}
