#include <stdio.h>
#include <string.h>

#define REPL_BUFSIZE 255

#include "metainfo.h"
#include "announce.h"

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
            print_announce();

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

