#include <stdio.h>
#include <string.h>

#define REPL_BUFSIZE 255

int repl(){
    int i;
    char input_buffer[REPL_BUFSIZE];
    
    for (i = 0; ; i++){
        printf("URTorrent (%d)> ", i);

        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL){
            perror("fgets");
            return 1;
        }

        if (!strcmp(input_buffer, "quit"))
            return 0;

        else if (!strcmp(input_buffer, "metainfo"))
            puts("metainfo not implemented yet");

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
    if (argc != 3)
        printf("URTorrent: a simple torrenting client\n\n"
               "Usage: urtorrent <port> <torrent_file>\n");

    return repl();
}
