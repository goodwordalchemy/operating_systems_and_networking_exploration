#include <openssl/sha.h>

#include "bencode/bencode.h"

#define HOSTNAME_BUFLEN 127
#define IP_BUFLEN 17
#define PORT_BUFLEN 6

be_node *metainfo;
be_node *trackerinfo;

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
    char announce_hostname[HOSTNAME_BUFLEN];
    char announce_port[PORT_BUFLEN];
    char *file_name;
    unsigned char info_hash[SHA_DIGEST_LENGTH + 1];
    char info_hash_digest[2*SHA_DIGEST_LENGTH + 1];
    char peer_id[SHA_DIGEST_LENGTH + 1];

    unsigned char **piece_hashes;
    char **piece_hash_digests;
};

struct localstate_t localstate;