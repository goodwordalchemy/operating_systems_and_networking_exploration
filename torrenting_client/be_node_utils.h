#include "bencode/bencode.h"

int index_of_key(be_node *node, char *key);

int get_be_node_int(be_node *node, char *key);

char *get_be_node_str(be_node *node, char *key);
