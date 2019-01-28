#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

void create_server_socket_listening_on_port(){}

void handle_incoming_connections(){}

int main(int argc, char *argv[]) {
    config_t config;
    char *config_filename;

    if (argc != 2){
        fprintf(stderr, "usage: pass the name of your config file.\n");
        return 1;
    }
    config_filename = argv[1];

    init_config(&config);

    parse_config_file(config_filename, &config);

    printf("about to print\n");
    print_config(&config);

    printf("about to handle fucking\n");
    create_server_socket_listening_on_port();

    handle_incoming_connections();

    printf("about to free config\n");
    free_config(&config);

    return 0;
}
