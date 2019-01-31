#include <math.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "metainfo.h"

#define FILE_SIZE_BUFLEN 50

int _get_file_length(char *filepath){
    struct stat buf;

    if (stat(filepath, &buf) == -1){
        perror("stat");
        return -1;
    }

    return (int) buf.st_size;
}

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

void _get_metainfo_hash(char *buffer){
    
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

    // To do:
    char *ip = "127.0.0.1";
    char *port = "8000";
    char *peer_id = "bcd914c766d969a772823815fdc2737b2c8384bf";
    char *info_hash = "4a060f199e5dc28ff2c3294f34561e2da423bf0b"; // calculated from metainfo

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

    printf("\tIP:port           : %s:%s\n", ip, port);
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
    int length;
    char *buffer;
    FILE *f;
    
    if ((length = _get_file_length(metainfo_filename)) < 1){
        fprintf(stderr, "There was an error finding the torrent file you've requested\n");
        return 1;
    }

    buffer = malloc(length);
    if (buffer == NULL){
        perror("malloc");
        return 1;
    }

    if ((f = fopen(metainfo_filename, "r")) == NULL){
        perror("fopen");
        return 1;
    }

    if ((fread(buffer, length, 1, f)) == 0){
        perror("fread");
        return 1;
    }

    if ((metainfo = be_decoden(buffer, 2*length)) == NULL){
        fprintf(stderr, "Could not decode metainfo file\n");
    };
    
    if (fclose(f) == EOF){
        perror("fclose");
        return 1;
    }

    free(buffer);

    return 0;
}

