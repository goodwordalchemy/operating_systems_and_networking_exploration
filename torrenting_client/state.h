#include <openssl/sha.h>

#include "bencode/bencode.h"

#define IP_BUFLEN 17
#define PORT_BUFLEN 6

be_node *metainfo;

struct localstate_t {
    // process state
    int is_seeder;
    char ip[IP_BUFLEN];
    char *client_port;
    char *metainfo_filename;

    // metainfo state
    int piece_length;
    int file_size;
	int n_pieces;
    int last_piece_size;

    char *announce_url;
    char *file_name;
    unsigned char info_hash[SHA_DIGEST_LENGTH + 1];
    char info_hash_digest[2*SHA_DIGEST_LENGTH + 1];
    char peer_id[2*SHA_DIGEST_LENGTH + 1];

    unsigned char **piece_hashes;
    char **piece_hash_digests;
};

struct localstate_t localstate;
