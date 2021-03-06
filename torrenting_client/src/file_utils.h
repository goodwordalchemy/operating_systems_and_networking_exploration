typedef struct filestring_t {
    char *data;
    int length;
} filestring_t;

filestring_t *read_file_to_string(char *filename);

void free_filestring(filestring_t *fs);

int get_file_length(char *filepath);

int does_file_exist(char *filename);
