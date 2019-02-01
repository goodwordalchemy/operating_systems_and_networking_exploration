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

    client_port = argv[1];
    metainfo_filename = argv[2];

    puts("Populating metainfo...");
    populate_metainfo();

    rc = repl();

    be_free(metainfo);

    return rc;
}
