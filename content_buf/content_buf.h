#ifndef CONTENT_BUF_HEADER
#define CONTENT_BUF_HEADER

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

struct content_buf {
  int cap_size;
  int size;
  char *buf;
};

static const int CONTENT_BUF_INIT_SIZE = 256;

int content_buf_destroy(struct content_buf *cb);
int content_buf_init   (struct content_buf *cb);
int content_buf_add    (struct content_buf *cb, char *fmt, ...);
int content_buf_print  (struct content_buf *cb, int fd);

#endif /* CONTENT_BUF_HEADER */
