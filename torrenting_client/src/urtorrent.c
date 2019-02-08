#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bitfield.h"
#include "metainfo.h"
#include "piece_trading.h"
#include "pieces.h"
#include "state.h"
#include "tracker.h"

void free_peers(){
    int i;
    
    for (i = 0; i < MAX_SOCKFD; i++){
        if (localstate.peers[i] != NULL){
            free_peer(i);
        }
    }
}

void cleanup(int trash){
    fprintf(stderr, "starting to clean up...\n");

    fprintf(stderr, "freeing peers...\n");
    free_peers();

    fprintf(stderr, "freeing piece names...\n");
    clean_pieces();

    fprintf(stderr, "freeing localstate metainfo...\n");
    free_localstate_metainfo();

    fprintf(stderr, "freeing my bitfield...\n");
    free(localstate.bitfield);

    fprintf(stderr, "freeing metainfo be_node...\n");
    if (metainfo != NULL)
        be_free(metainfo);

    fprintf(stderr, "freeing trackerinfo be_node...\n");
    if (trackerinfo != NULL)
        be_free(trackerinfo);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3){
        printf("URTorrent: a simple torrenting client\n\n"
               "Usage: urtorrent <port> <torrent_file>\n\n");
        exit(0);
    }

    localstate.client_port = argv[1];
    localstate.metainfo_filename = argv[2];

    if (signal(SIGINT, cleanup) == SIG_ERR)
        perror("signal");

    printf("Parsing torrent file...\n");
    print_metainfo();

    if (localstate.is_seeder){
        printf("You are a seeder.  Creating pieces.\n");
        if (create_pieces() == -1)
            cleanup(0);
    }

    printf("Registering with tracker...\n");
    print_announce();

    printf("Storing my bitfield...\n");
    store_my_bitfield();

    localstate.uploaded = 0;

    printf("Downloading pieces from peers...\n");
    setup_peer_connections();

    printf("Cleaning up...\n");
    cleanup(0);

    return 0;
}
