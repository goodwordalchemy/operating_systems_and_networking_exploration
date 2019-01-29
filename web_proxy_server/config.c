#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define LINE_BUFFER_LENGTH 255
#define TOKENS_PER_LINE 63
#define SPACE_DELIMETERS  " \n\r"
#define PORT_BUFLEN 8

site_list_node *init_site_list_node(char *blocked_site){
    site_list_node *new_node = malloc(sizeof(site_list_node));
    if (new_node == NULL){
        perror("malloc");
        return NULL;
    }

    new_node->site = strdup(blocked_site);

    if (new_node->site == NULL){
        perror("strdup");
        return NULL;
    }

    new_node->next = NULL;
    return new_node;
}

void free_site_list_node(site_list_node *cur_node){
    if (cur_node == NULL)
        return;

    site_list_node *next_node;
    next_node = cur_node->next;
    free(cur_node);
    while(next_node != NULL){
        cur_node = next_node;
        next_node = cur_node->next;
        free(cur_node->site);
        free(cur_node);
    }
}

void init_config(config_t *config){
    config->blocked_sites = NULL;
    config->last_blocked_site = NULL;
	config->port = malloc(sizeof(char) * PORT_BUFLEN);
}

void free_config(config_t *config){
    free_site_list_node(config->blocked_sites);
	free(config->port);
}

void add_blocked_site(config_t *config, char *blocked_site){
    site_list_node *new_node;

    new_node = init_site_list_node(blocked_site);
    if (new_node == NULL){
        free_config(config);
        exit(EXIT_FAILURE);
    }


    if (config->blocked_sites == NULL){
        config->blocked_sites = new_node;
        config->last_blocked_site = new_node;
    }
    else {
        config->last_blocked_site->next = new_node;
        config->last_blocked_site = new_node;
    }
}

int tokenize_line(char *line, char **tokens){
    char *token, *saveptr;
    int num_tokens = 0;

    memset(tokens, 0, TOKENS_PER_LINE);
    token = strtok_r(line, SPACE_DELIMETERS, &saveptr);

    while(token != NULL){
        tokens[num_tokens] = token;
        num_tokens++;
        token = strtok_r(NULL, SPACE_DELIMETERS, &saveptr);
    }

    return num_tokens;
}

void parse_config_file(char *config_filename, config_t *config){
    int num_tokens;
    FILE *config_fp;
    char line_buffer[LINE_BUFFER_LENGTH];
    char *line_tokens[TOKENS_PER_LINE];

    if ((config_fp = fopen(config_filename, "r")) == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    while (fgets(line_buffer, LINE_BUFFER_LENGTH, config_fp) != NULL){
        num_tokens = tokenize_line(line_buffer, line_tokens);
        
        if (num_tokens == 0)
            continue;

        if (strncmp("#", line_tokens[0], 1) == 0)
            continue;

        else if (strcmp("port", line_tokens[0]) == 0)
			strcpy(config->port, line_tokens[1]);

        else if (strcmp("block", line_tokens[0]) == 0)
            add_blocked_site(config, line_tokens[1]);

        else {
            fprintf(stderr, "Unrecognized config key in line: '%s'", line_buffer);
            free_config(config);
            exit(EXIT_FAILURE);
        }
    }

}

void print_config(config_t *config){
    printf("Configuration: \n");
    site_list_node *cur_node, *next_node;


    printf("->port: %s\n", config->port);

    cur_node = config->blocked_sites;
    if (cur_node == NULL)
        return;
    next_node = cur_node->next;
    printf("->blocked site: %s\n", cur_node->site);
    while(next_node != NULL){
        cur_node = next_node;
        printf("->blocked site: %s\n", cur_node->site);
        next_node = cur_node->next;
    }
    printf("\n");
}
