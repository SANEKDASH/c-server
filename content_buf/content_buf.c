#include "content_buf.h"

static int content_buf_realloc(struct content_buf *cb, size_t new_size);

int content_buf_init(struct content_buf *cb)
{
  cb->cap_size = CONTENT_BUF_INIT_SIZE;
  cb->size = 0;

  cb->buf = (char *) malloc(sizeof(char) * cb->cap_size); 

  if (cb->buf == NULL) {
	perror("failed to alloc content buffer");
	return -1;
  }
  
  return 0;
}

int content_buf_destroy(struct content_buf *cb)
{  
  cb->cap_size = 0;
  cb->size = 0;

  free(cb->buf);
 
  return 0;
}

static int content_buf_realloc(struct content_buf *cb, size_t new_size)
{
  cb->cap_size = new_size;

  cb->buf = (char *) realloc(cb->buf, cb->cap_size * sizeof(char));
  
  if (cb->buf == NULL) {
	perror("failed to realloc content_buf");
	return -1;
  }
  
  return 0;
}

int content_buf_add(struct content_buf *cb, char *fmt, ...)
{
  va_list arg_list;
  va_start(arg_list, fmt);

  if (cb->size >= cb->cap_size) {
	  content_buf_realloc(cb, cb->cap_size + CONTENT_BUF_INIT_SIZE);	
  }
  
  while (1) {
	va_list arg_copy;
	va_copy(arg_copy, arg_list);
	int size = vsnprintf(cb->buf + cb->size, cb->cap_size - cb->size, fmt, arg_copy);

	if (size >= cb->cap_size - cb->size) {
	  content_buf_realloc(cb, cb->cap_size + CONTENT_BUF_INIT_SIZE);
	} else {
	  cb->size += size;
	  va_end(arg_copy);
	  break;
	}
	va_end(arg_copy);
  }
  
  va_end(arg_list);

  return 0;
}

int content_buf_print(struct content_buf *cb, int fd)
{
  dprintf(fd, "%s", cb->buf);
  
  return 0;
}

