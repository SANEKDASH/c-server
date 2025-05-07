#ifndef DNT_NODE_HEADER
#define DNT_NODE_HEADER

#include <stdlib.h>

struct dirent_node {
  struct dirent *dnt;
  struct dirent_node *next;
};

struct dirent_node *dirent_node_alloc(struct dirent *dir_entry);
int dirent_node_destroy(struct dirent_node *dnt_node);

#endif
