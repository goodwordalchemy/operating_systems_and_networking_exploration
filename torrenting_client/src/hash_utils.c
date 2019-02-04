#include <openssl/sha.h>
#include <stdio.h>

#include "hash_utils.h"

void hex_digest(unsigned char *hash, char *buffer){
    int j;

    for (j = 0; j < SHA_DIGEST_LENGTH; j++){
        snprintf(buffer + (j*2), 3,
                "%02x", *(hash + j));
    }
}

