#include "cxn_ctx.h"

int cxn_ctx_init(struct cxn_ctx *cxn_ctx, int cxn_fd)
{
  if (cxn_fd < 0) {
	return -1;
  }

  cxn_ctx->dir = NULL;
  cxn_ctx->cxn_fd = cxn_fd;
  cxn_ctx->cxn_file = fdopen(cxn_ctx->cxn_fd, "r");

  if (cxn_ctx->cxn_file == NULL) {
	return -1;
  }
  
  cxn_ctx->dnt_node = NULL;
  
  cxn_ctx->file_fd = -1;
    
  memset(&cxn_ctx->req_info, 0, sizeof(cxn_ctx->req_info));

  content_buf_init(&cxn_ctx->req);
  content_buf_init(&cxn_ctx->resp);
  
  return 0;
}

int cxn_ctx_destroy(struct cxn_ctx *cxn_ctx)
{
  content_buf_destroy(&cxn_ctx->req);
  content_buf_destroy(&cxn_ctx->resp);

  fclose(cxn_ctx->cxn_file);

  if (cxn_ctx->dir != NULL) {
	closedir(cxn_ctx->dir);
  }

  dirent_node_destroy(cxn_ctx->dnt_node);

  cxn_ctx->dnt_node = NULL;
  
  return 0;
}
