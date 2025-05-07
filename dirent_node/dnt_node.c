#include "dnt_node.h"

struct dirent_node *dirent_node_alloc(struct dirent *dir_entry)
{
  struct dirent_node *dnt_node_ptr = (struct dirent_node *) malloc(sizeof(struct dirent_node));

  if (dnt_node_ptr == NULL) {
	return NULL;
  }
  
  dnt_node_ptr->next = NULL;
  dnt_node_ptr->dnt = dir_entry;
  
  return dnt_node_ptr;
}

int dirent_node_destroy(struct dirent_node *dnt_node)
{
  struct dirent_node *cur_node = dnt_node;

  while (cur_node != NULL) {
	struct dirent_node *next_node = cur_node->next;
	free(cur_node);
	cur_node = next_node;
  }
  
  return 0;
}
