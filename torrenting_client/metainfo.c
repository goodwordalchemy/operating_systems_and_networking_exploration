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

int print_metainfo(){
    int piece_length;
    char *file_name; // in metainfo ...
    char file_size[FILE_SIZE_BUFLEN];

    // To do:
    char *ip = "127.0.0.1";
    char *port = "8000";
    char *peer_id = "bcd914c766d969a772823815fdc2737b2c8384bf";
    char *info_hash = "4a060f199e5dc28ff2c3294f34561e2da423bf0b"; // calculated from metainfo
    char *announce_url;

    char *piece_hashes[] = {
		"064b493d90b6811f22e0457aa7f50e9c70b84285",
		"d17cb90e50ca06a651a84f88fde0ecfb22a90cca",
		"20e82d045341032645ebe27eed38103329281175",
		"568c8a0599a7c1e2b3c70d8b8c960134653d497a"
	};
	int n_pieces = 4;
    int i;
    
    file_name = _get_info_node_str("name");
    piece_length = _get_info_node_int("piece length");
    _write_file_size_str(file_size);
    announce_url = _get_announce_url(); 


    printf("\tIP/portt          : %s/%s\n", ip, port);
    printf("\tID                : %s\n", peer_id);
    printf("\tmetainfo file     : %s\n", metainfo_filename);
    printf("\tinfo hash         : %s\n", info_hash);
    printf("\tfile nam          : %s\n", file_name);
    printf("\tpiece length      : %d\n", piece_length);
    printf("\tfile size         : %s\n", file_size);
    printf("\tannounce URL      : %s\n", announce_url);

    printf("\tpiece hashes      :\n");
    for (i = 0; i < n_pieces; i++)
        printf("\t\t%d %s\n", i, piece_hashes[i]);

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

