typedef struct __site_list_node {
    char *site;
    struct __site_list_node *next;
    
} site_list_node;


typedef struct __config_t {
    char* port;
    site_list_node *blocked_sites;
    site_list_node *last_blocked_site;
} config_t;

void init_config(config_t *config);

void free_config(config_t *config);

void parse_config_file(char *config_filename, config_t *config);

void print_config(config_t *config);
