#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "announce.h"
#include "metainfo.h"
#include "state.h"

int main(int argc, char *argv[]) {
    int rc;

    if (argc != 3){
        printf("URTorrent: a simple torrenting client\n\n"
               "Usage: urtorrent <port> <torrent_file>\n\n");
        exit(0);
    }

    localstate.client_port = argv[1];
    localstate.metainfo_filename = argv[2];

    printf("Parsing torrent file...");
    print_metainfo();

    printf("Registering with tracker...");
    print_announce();

    free_localstate_metainfo();
    be_free(metainfo);
    be_free(trackerinfo);

    return rc;
}
