#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "state.h"
#include "metainfo.h"
#include "repl.h"


int main(int argc, char *argv[]) {
    int rc;

    if (argc != 3){
        printf("URTorrent: a simple torrenting client\n\n"
               "Usage: urtorrent <port> <torrent_file>\n\n");
        exit(0);
    }

    localstate.client_port = argv[1];
    localstate.metainfo_filename = argv[2];

    puts("Setting up client state...");
    setup_metainfo();

    rc = repl();

    free_localstate_metainfo();
    be_free(metainfo);

    return rc;
}
