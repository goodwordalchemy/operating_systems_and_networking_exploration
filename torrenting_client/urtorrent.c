#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bencode/bencode.h"

#define REPL_BUFSIZE 255

char *metainfo_filename;
char *client_port;
be_node *metainfo;

int _get_file_length(char *filepath){
    struct stat buf;

    if (stat(filepath, &buf) == -1){
        perror("stat");
        return -1;
    }

    return (int) buf.st_size;
}

int print_metainfo(){
    puts("this is not implemented yet");
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

    if ((metainfo = be_decode(buffer)) == NULL){
        fprintf(stderr, "Could not decode metainfo file\n");
    };
    
    printf("%s", buffer);

    if (fclose(f) == EOF){
        perror("fclose");
        return 1;
    }

    return 0;
}

int repl(){
    int i;
    char input_buffer[REPL_BUFSIZE];
    
    for (i = 0; ; i++){
        printf("URTorrent (%d)> ", i);

        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL){
            perror("fgets");
            return 1;
        }
        input_buffer[strcspn(input_buffer, "\n")] = 0;

        if (!strlen(input_buffer))
            continue;

        else if (!strcmp(input_buffer, "quit"))
            return 0;

        else if (!strcmp(input_buffer, "metainfo"))
            print_metainfo();

        else if (!strcmp(input_buffer, "announce"))
            puts("announce not implemented yet");

        else if (!strcmp(input_buffer, "trackerinfo"))
            puts("trackerinfo not implemented yet");

        else if (!strcmp(input_buffer, "show"))
            puts("show not implemented yet");

        else if (!strcmp(input_buffer, "status"))
            puts("status not implemented yet");

        else
            printf("Error: unrecognized command\n");
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3){
        printf("URTorrent: a simple torrenting client\n\n"
               "Usage: urtorrent <port> <torrent_file>\n\n");
        exit(0);
    }

    client_port = argv[1];
    metainfo_filename = argv[2];

    return repl();
}
