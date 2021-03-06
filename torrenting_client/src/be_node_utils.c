#include <string.h>

#include "be_node_utils.h"

int index_of_key(be_node *node, char *key){
    int i;

    for (i = 0; node->val.d[i].val; ++i) {
        if (!strcmp(node->val.d[i].key, key))
            return i;
    }
    return -1;
}

int get_be_node_int(be_node *node, char *key){
    int res, idx;
    if ((idx = index_of_key(node, key)) == -1)
        return -1;

    res = (int) node->val.d[idx].val->val.i;

    return res;
}

char *get_be_node_str(be_node *node, char *key){
    int idx;

    if ((idx = index_of_key(node, key)) == -1)
        return NULL;

    return node->val.d[idx].val->val.s;
}
