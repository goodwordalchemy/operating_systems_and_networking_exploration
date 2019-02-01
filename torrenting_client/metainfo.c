#include <math.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "filestring.h"
#include "ip_address.h"
#include "metainfo.h"

#define FILE_SIZE_BUFLEN 50
#define IP_BUFLEN 17


int _index_of_key(be_node *node, char *key){
    int i;

    for (i = 0; node->val.d[i].val; ++i) {
        if (!strcmp(node->val.d[i].key, key))
            return i;
    }
    return -1;
}

char *_get_announce_url(){
    int idx;

    idx = _index_of_key(metainfo, "announce");

    return metainfo->val.d[idx].val->val.s;
}

be_node *get_info_node(){
    int idx;
    be_dict *metainfo_dict;
    be_node *info_node;

    idx = _index_of_key(metainfo, "info");

    metainfo_dict= metainfo->val.d;

    info_node = metainfo_dict[idx].val;

    return info_node;
}

int _get_info_node_int(char *key){
    int idx, res;
    be_node *info_node;

    info_node = get_info_node();
    idx = _index_of_key(info_node, key);

    res = (int) info_node->val.d[idx].val->val.i;

    return res;
}

char *_get_info_node_str(char *key){
    int idx;
    be_node *info_node;

    info_node = get_info_node();
    idx = _index_of_key(info_node, key);

    return info_node->val.d[idx].val->val.s;
}

void _write_file_size_str(char *buf){
    int quotient, file_length, piece_length, remainder;

    file_length = _get_info_node_int("length");
    piece_length = _get_info_node_int("piece length");

    quotient = file_length / piece_length;
    remainder = file_length % piece_length;

    snprintf(buf, FILE_SIZE_BUFLEN, "%d (%d * [piece length] + %d)", 
             file_length,quotient, remainder); 
}

void _free_hash_pieces_array(char **hpa, int size){
    int i;

    for (i = 0; i < size; i++)
        free(hpa[i]);

    free(hpa);
}

void _hex_digest(char *hash, char *buffer){
    int j;

    for (j = 0; j < SHA_DIGEST_LENGTH; j++){
        snprintf(buffer + (j*2), 3,
                "%02x", 128 + *(hash + j));
    }
}

char* _get_metainfo_hash(char *metadata_buffer){
    char *substr;
    unsigned char hash[SHA_DIGEST_LENGTH];
    filestring_t *fs;

    fs = read_file_to_string(metainfo_filename);

    if ((substr = strstr(fs->data, "4:info")) == NULL){
        fprintf(stderr, "Could not find info dictionary in metafile");
        return NULL;
    }
    substr += strlen("4:info");


    // -3 because -1 for null and -2 for e's at end of info dict.
    // another way to do this is commented out below.
    // Note that this other way is also fine because the string
    // they are modifying is about to be freed.
    /* substr[strlen(substr)-1] = 0; */
    /* substr[strlen(substr)-2] = 0; */
    SHA1((unsigned char*) substr, sizeof(substr) - 3, hash); 

    printf("the hash: %s\n", hash);

    _hex_digest((char *) hash, metadata_buffer);
    
    free_filestring(fs);

    return NULL;
}

char **_get_piece_hashes_array(int n_pieces){
    int i;
    char *pieces;
    char **piece_hashes;
    char *buffer;

    pieces = _get_info_node_str("pieces");
    piece_hashes = malloc(sizeof(char*) * n_pieces);

    for (i = 0; i < n_pieces; i++){
        buffer = malloc(sizeof(char) * SHA_DIGEST_LENGTH*2 + 1);

        _hex_digest(pieces + (i * SHA_DIGEST_LENGTH), buffer);

        piece_hashes[i] = buffer;
    }

    return piece_hashes;
}

int print_metainfo(){
    int i;
    int piece_length;
    int file_length;
	int n_pieces;
    int last_piece_size;

    char *announce_url;
    char *file_name; // in metainfo ...
    char file_size_str[FILE_SIZE_BUFLEN];
    char info_hash[40];
    char ip[IP_BUFLEN];

    // To do:
    char *peer_id = "bcd914c766d969a772823815fdc2737b2c8384bf";

    char **piece_hashes;
    
    file_name = _get_info_node_str("name");
    announce_url = _get_announce_url(); 
    piece_length = _get_info_node_int("piece length");
    
    file_length = _get_info_node_int("length");
    n_pieces = ceil(file_length / piece_length);
    last_piece_size = file_length % piece_length;
    snprintf(file_size_str, FILE_SIZE_BUFLEN, "%d (%d * [piece length] + %d)", 
             file_length, n_pieces, last_piece_size); 

    piece_hashes = _get_piece_hashes_array(n_pieces);
    _get_metainfo_hash(info_hash);

    get_local_ip_address(ip, IP_BUFLEN); 

    printf("\tIP:port           : %s:%s\n", ip, client_port);
    printf("\tID                : %s\n", peer_id);
    printf("\tmetainfo file     : %s\n", metainfo_filename);
    printf("\tinfo hash         : %s\n", info_hash);
    printf("\tfile name         : %s\n", file_name);
    printf("\tpiece length      : %d\n", piece_length);
    printf("\tfile size         : %s\n", file_size_str);
    printf("\tannounce URL      : %s\n", announce_url);

    printf("\tpiece hashes      :\n");
    for (i = 0; i < n_pieces; i++)
        printf("\t\t%2d\t%s\n", i, piece_hashes[i]);

    _free_hash_pieces_array(piece_hashes, n_pieces);

    return 0;
}


int populate_metainfo(){
    filestring_t *fs;
    
    fs = read_file_to_string(metainfo_filename);

    if ((metainfo = be_decoden(fs->data, 2*(fs->length))) == NULL){
        fprintf(stderr, "Could not decode metainfo file\n");
    };
    
    free_filestring(fs);

    return 0;
}

