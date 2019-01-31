#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "metainfo.h"

#define REPL_BUFSIZE 255

char *metainfo_filename;
char *client_port;

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
    int rc;

    if (argc != 3){
        printf("URTorrent: a simple torrenting client\n\n"
               "Usage: urtorrent <port> <torrent_file>\n\n");
        exit(0);
    }

    client_port = argv[1];
    metainfo_filename = argv[2];

    puts("Populating metainfo...");
    populate_metainfo();

    rc = repl();

    free(metainfo);

    return rc;
}
