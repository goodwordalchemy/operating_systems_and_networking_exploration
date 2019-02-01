#include <errno.h>
#include <math.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "state.h"
#include "filestring.h"
#include "ip_address.h"
#include "metainfo.h"

#define FILE_SIZE_BUFLEN 50

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
    int quotient, file_size, piece_length, remainder;

    file_size = _get_info_node_int("length");
    piece_length = _get_info_node_int("piece length");

    quotient = localstate.file_size / localstate.piece_length;
    remainder = localstate.file_size % localstate.piece_length;

    snprintf(buf, FILE_SIZE_BUFLEN, "%d (%d * [piece length] + %d)", 
             localstate.file_size, quotient, remainder); 
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

void _hash_and_get_digest(unsigned char *str, int to_offset, char *buf){
    unsigned char hash[SHA_DIGEST_LENGTH];

    SHA1(str, to_offset, hash); 

    _hex_digest((char*)hash, (char*)buf);
}

char *get_infodict_str(char *buffer){
    char *substr;
    filestring_t *fs;

    fs = read_file_to_string(localstate.metainfo_filename);

    if ((substr = strstr(fs->data, "4:info")) == NULL){
        fprintf(stderr, "Could not find info dictionary in metafile");
        return NULL;
    }
    substr += strlen("4:info");

    // -2 for e's at end of info dict.
    memcpy(buffer, substr, strlen(substr)-2);
    
    free_filestring(fs);

    return buffer;
}

char* _get_infodict_digest(char *metadata_buffer){
    // TODO get rid of VLA
    int bufsize = get_file_length(localstate.metainfo_filename) - 8;
    char infodict_str_buf[bufsize];

    get_infodict_str(infodict_str_buf);

    _hash_and_get_digest((unsigned char*)infodict_str_buf, bufsize, metadata_buffer);

    return metadata_buffer;
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

void _get_peer_id(char *buf){
   char concat[IP_BUFLEN + PORT_BUFLEN]; 

   snprintf(concat, IP_BUFLEN + PORT_BUFLEN, "%s%s", 
            localstate.ip, localstate.client_port);

   _hash_and_get_digest((unsigned char*)concat, sizeof(concat)-1, buf);

}

char *_get_recommended_filename(){
    return _get_info_node_str("name");
}

int _get_piece_length(){
    return _get_info_node_int("piece length");
}

int _get_file_size(){
    return _get_info_node_int("length");
}


int print_metainfo(){
    int i;
    char file_size_str[FILE_SIZE_BUFLEN];

    _write_file_size_str(file_size_str);

    printf("\tIP:port           : %s:%s\n", localstate.ip, localstate.client_port);
    printf("\tID                : %s\n", localstate.peer_id);
    printf("\tmetainfo file     : %s\n", localstate.metainfo_filename);
    printf("\tinfo hash         : %s\n", localstate.info_hash);
    printf("\tfile name         : %s\n", localstate.file_name);
    printf("\tpiece length      : %d\n", localstate.piece_length);
    printf("\tfile size         : %s\n", file_size_str);
    printf("\tannounce URL      : %s\n", localstate.announce_url);

    printf("\tpiece hashes      :\n");
    for (i = 0; i < localstate.n_pieces; i++)
        printf("\t\t%2d\t%s\n", i, localstate.piece_hashes[i]);

    return 0;
}

void _populate_is_seeder(){
    char *rec_filename;
    struct stat buf;

    rec_filename = _get_recommended_filename();
    if (stat(rec_filename, &buf) == 0)
        localstate.is_seeder = 1;
    else if (errno == ENOENT)
        localstate.is_seeder = 0;

    else{
        perror("stat");
        fprintf(stderr, "Error: Could not determine if you are a seeder or leecher.\n");
        localstate.is_seeder = 0;
    }
}

int populate_metainfo(){
    filestring_t *fs;
    
    fs = read_file_to_string(localstate.metainfo_filename);

    if ((metainfo = be_decoden(fs->data, 2*(fs->length))) == NULL){
        fprintf(stderr, "Could not decode metainfo file\n");
    };
    
    free_filestring(fs);


    return 0;
}

void populate_localstate_metainfo(){
    _populate_is_seeder();
    localstate.file_name = _get_recommended_filename();
    localstate.announce_url = _get_announce_url(); 
    localstate.piece_length = _get_piece_length();
    
    localstate.file_size = _get_file_size();
    localstate.n_pieces = ceil(localstate.file_size / localstate.piece_length);
    localstate.last_piece_size = localstate.file_size % localstate.piece_length;

    localstate.piece_hashes = _get_piece_hashes_array(localstate.n_pieces);
    _get_infodict_digest(localstate.info_hash_digest);

    get_local_ip_address(localstate.ip, IP_BUFLEN); 
    _get_peer_id(localstate.peer_id);
}

void setup_metainfo(){
    populate_metainfo();
    populate_localstate_metainfo();
}
