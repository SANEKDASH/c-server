#ifndef CXN_CTX_HEADER
#define CXN_CTX_HEADER

#include <unistd.h>
#include <string.h>
#include <dirent.h>

#include "../content_buf/content_buf.h"
#include "../dirent_node/dnt_node.h"

struct req_info {
  char *req_type;
  char *path;
  char *html_ver;
};

struct cxn_ctx {
  const char *ip_addr;
  int port;
  
  int cxn_fd;
  FILE *cxn_file;
  
  int file_fd;
  DIR *dir;
  struct dirent_node *dnt_node;
  
  struct req_info req_info;

  struct content_buf req;
  struct content_buf resp;
};

int cxn_ctx_init   (struct cxn_ctx *cxn_ctx, int cxn_fd, const char *ip_addr, const int port);
int cxn_ctx_destroy(struct cxn_ctx *cxn_ctx);

#endif
