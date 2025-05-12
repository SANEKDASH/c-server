#include "cxn_queue.h"

static inline int next_pos(int cur_pos);

static inline int next_pos(int cur_pos)
{
  return (cur_pos + 1) % BACKLOG_COUNT;
}

int connection_queue_init(struct connection_queue *cxn_queue)
{
  cxn_queue->head = 0;
  cxn_queue->tail = 0;

  return 0;
}

int connection_queue_add(struct connection_queue *cxn_queue,
						 int cxn_fd, const char *ip_addr, const int port)
{
  
  if (next_pos(cxn_queue->head) == cxn_queue->tail) {
	printf("HUYATINA\n");
	return -1;
  }
  
  if (cxn_ctx_init(&cxn_queue->ctx[cxn_queue->head], cxn_fd, ip_addr, port) < 0) {
	printf("PIDR\n");
	return -1;
  }  

  cxn_queue->head = next_pos(cxn_queue->head);
  
  return 0;
}

int connection_queue_pop(struct connection_queue *cxn_queue)
{  
  if (cxn_queue->tail == cxn_queue->head) {
	return -1;
  }
  
  cxn_ctx_destroy(&cxn_queue->ctx[cxn_queue->tail]);
  
  cxn_queue->tail = next_pos(cxn_queue->tail);

  return 0;
}

int connection_queue_destroy(struct connection_queue *cxn_queue)
{
  int res = 0;

  do {
	res = connection_queue_pop(cxn_queue);
  } while (res == 0);
  
  return 0;
}

inline struct cxn_ctx *connection_queue_get_tail(struct connection_queue *cxn_queue)
{
  return &cxn_queue->ctx[cxn_queue->tail];
}

struct cxn_ctx *connection_queue_find(struct connection_queue *cxn_queue, int fd)
{
  for (int i = cxn_queue->tail; i !=  cxn_queue->head; i = next_pos(i)) {
	if (fd == cxn_queue->ctx[i].cxn_fd) {
	  return &cxn_queue->ctx[i];
	}
  }
  
  return NULL;
}
