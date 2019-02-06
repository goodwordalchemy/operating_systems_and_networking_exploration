#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "file_utils.h"
#include "hash_utils.h"
#include "state.h"

#define HEX_DIGEST_BUFLEN ((SHA_DIGEST_LENGTH * 2) + 1)

int validate_piece(char *piece_digest, char *data, int length){
    int i;
    unsigned char hash_buf[SHA_DIGEST_LENGTH+1];
    char digest_buf[HEX_DIGEST_BUFLEN];


    SHA1((unsigned char *)data, length, hash_buf); 


    hex_digest(hash_buf, digest_buf);

    for (i = 0; i < HEX_DIGEST_BUFLEN - 1; i++){
        if (piece_digest[i] != digest_buf[i])
            return 0;
    }

    return 1;
}

int validate_saved_piece(char *piece_digest){
    int result;
    filestring_t *fs;

    fs = read_file_to_string(piece_digest);

    result = validate_piece(piece_digest, fs->data, fs->length);

    free(fs);

    return result;
}

int create_pieces(){
    int i, nbytes;
    FILE *target, *tmp;
    int piece_size = localstate.piece_length;
    char buf[piece_size + 1];
    char *cur_piece_digest;

    target = fopen(localstate.file_name, "r");


    for (i = 0; i < localstate.n_pieces; i++){
        memset(buf, 0, piece_size + 1);

        cur_piece_digest = localstate.piece_hash_digests[i];
        if ((tmp = fopen(cur_piece_digest, "w")) == NULL){
            perror("fopen while sharding target");
            return -1;
        }

         /* If your file is less than 1000 bytes, fread(a, 1, 1000, stdin) 
          * (read 1000 elements of 1 byte each) will still copy all the bytes 
          * until EOF. On the other hand, the result of fread(a, 1000, 1, stdin) 
          * (read 1 1000-byte element) stored in a is unspecified, because there is 
          * not enough data to finish reading the 'first' (and only) 1000 byte element. */
        if ((nbytes = fread(buf, 1, piece_size, target)) == piece_size);
        else if (nbytes == localstate.last_piece_size);
        else {
            perror("fread while sharding target");
            fclose(tmp);
            return -1;
        }

        if ((nbytes = fwrite(buf, 1, piece_size, tmp)) == piece_size);
        else if (nbytes == localstate.last_piece_size);
        else {
            perror("fwrite while sharding target");
            fclose(tmp);
            return -1;
        }

        if (!validate_saved_piece(cur_piece_digest)){
            fprintf(stderr, "Could not validate piece.\n");
            return -1;
        }



        fclose(tmp);
    }

    fclose(target);

    return 0;
}

void clean_pieces(){
    int i;
    char *cur_digest; 

    for (i = 0; i < localstate.n_pieces; i++){
        cur_digest = localstate.piece_hash_digests[i];
        if (does_file_exist(cur_digest) && unlink(cur_digest) < 0)
            perror("unlink");
    }
}
