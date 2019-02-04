#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "announce.h"
#include "metainfo.h"
#include "handshake.h"
#include "state.h"

void free_peers(){
    int i;
    
    for (i = 0; i < MAX_SOCKFD; i++){
        if (localstate.peers[i] != NULL)
            free(localstate.peers[i]);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3){
        printf("URTorrent: a simple torrenting client\n\n"
               "Usage: urtorrent <port> <torrent_file>\n\n");
        exit(0);
    }

    localstate.client_port = argv[1];
    localstate.metainfo_filename = argv[2];

    printf("Parsing torrent file...\n");
    print_metainfo();

    printf("Registering with tracker...\n");
    print_announce();

    printf("Downloading pieces from peers...\n");
    setup_peer_connections();

    free_peers();
    free_localstate_metainfo();
    be_free(metainfo);
    be_free(trackerinfo);

    return 0;
}
