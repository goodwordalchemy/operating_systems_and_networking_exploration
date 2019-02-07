#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "file_utils.h"

int does_file_exist(char *filename){
    struct stat buf;
    if (stat(filename, &buf) == -1){
        if (errno != ENOENT)
            perror("stat");
        return 0;
    }
    return 1;
}


int get_file_length(char *filepath){
    struct stat buf;

    if (stat(filepath, &buf) == -1){
        perror("stat");
        return -1;
    }

    return (int) buf.st_size;
}

filestring_t *read_file_to_string(char *filename){
    filestring_t *fs;
    int length;
    char *buffer;
    FILE *f;
    
    if ((length = get_file_length(filename)) < 1){
        fprintf(stderr, "There was an error finding the torrent file you've requested\n");
        return NULL;
    }

    buffer = malloc(length);
    if (buffer == NULL){
        perror("malloc");
        return NULL;
    }

    if ((f = fopen(filename, "r")) == NULL){
        perror("fopen");
        return NULL;
    }

    if ((fread(buffer, length, 1, f)) == 0){
        perror("fread");
        return NULL;
    }
    
    if (fclose(f) == EOF){
        perror("fclose");
        return NULL;
    }

    fs = malloc(sizeof(filestring_t));
    fs->length = length;
    fs->data = buffer;

    return fs;
}

void free_filestring(filestring_t *fs){
    free(fs->data);
    free(fs);
}
