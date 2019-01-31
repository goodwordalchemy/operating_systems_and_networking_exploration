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
    char *ip = "127.0.0.1";
    char *port = "8000";
    char *peer_id = "bcd914c766d969a772823815fdc2737b2c8384bf";
    char *info_hash = "4a060f199e5dc28ff2c3294f34561e2da423bf0b"; // calculated from metainfo
    char *file_name = "lovely_photo.jpg"; // in metainfo ...
    char *piece_length = "262144";
    char *file_size = "1007616 (3 * [piece length] + 221184)"; 
    char *announce_url = "http://192.168.0.1:9999/announce";

    char *piece_hashes[] = {
		"064b493d90b6811f22e0457aa7f50e9c70b84285",
		"d17cb90e50ca06a651a84f88fde0ecfb22a90cca",
		"20e82d045341032645ebe27eed38103329281175",
		"568c8a0599a7c1e2b3c70d8b8c960134653d497a"
	};
	int n_pieces = 4;
    int i;
    


    printf("\tIP/port\t\t\t\t : %s/%s\n", ip, port);
    printf("\tID\t\t\t\t\t: %s\n", peer_id);
    printf("\tmetainfo file\t\t\t\t: %s\n", metainfo_filename);
    printf("\tinfo hash\t\t\t\t: %s\n", info_hash);
    printf("\tfile name\t\t\t\t: %s\n", file_name);
    printf("\tpiece length\t\t\t : %s\n", piece_length);
    printf("\tfile size\t\t\t\t: %s\n", file_size);
    printf("\tannounce URL\t\t\t\t: %s\n", announce_url);

    printf("\tpiece hashes\t\t\t\t:\n");
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

    if ((metainfo = be_decoden(buffer, length)) == NULL){
        fprintf(stderr, "Could not decode metainfo file\n");
    };
    
    if (fclose(f) == EOF){
        perror("fclose");
        return 1;
    }

    free(buffer);

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

    puts("Populating metainfo...");
    populate_metainfo();

    return repl();
}
