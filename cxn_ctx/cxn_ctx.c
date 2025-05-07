#include "cxn_ctx.h"

int cxn_ctx_init(struct cxn_ctx *cxn_ctx, int cxn_fd)
{
  cxn_ctx->fd = cxn_fd;
  
  cxn_ctx->dnt_node = dirent_node_alloc(NULL);
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

  close(cxn_ctx->fd);
  
  return 0;
}
