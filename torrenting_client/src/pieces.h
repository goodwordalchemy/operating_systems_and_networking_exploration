#define HEX_DIGEST_BUFLEN ((SHA_DIGEST_LENGTH * 2) + 1)
#define EXTENSION_LENGTH 6
#define FILENAME_WITH_EXT_BUFLEN (HEX_DIGEST_BUFLEN + EXTENSION_LENGTH)

int create_pieces();

void clean_pieces();

int validate_piece(char *piece_digest, char *data, int length);

int does_piece_exist(char *piece_digest);

void get_filename_with_extension(char *piece_digest, char *buf);
