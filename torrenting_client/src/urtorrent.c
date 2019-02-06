#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "announce.h"
#include "metainfo.h"
#include "handshake.h"
#include "pieces.h"
#include "state.h"

void free_peers(){
    int i;
    
    for (i = 0; i < MAX_SOCKFD; i++){
        if (localstate.peers[i] != NULL)
            free(localstate.peers[i]);
    }
}

void cleanup(int trash){
    clean_pieces();
    free_peers();
    free_localstate_metainfo();

    if (metainfo != NULL)
        be_free(metainfo);

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

    printf("Downloading pieces from peers...\n");
    setup_peer_connections();

    printf("Cleaning up...\n");
    cleanup(0);

    return 0;
}
