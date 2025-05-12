#ifndef CXN_ARR_HEADER
#define CXN_ARR_HEADER

#include "../common.h"
#include "../cxn_ctx/cxn_ctx.h"

struct connection_queue {
  int head;
  int tail;
  pthread_t threads[BACKLOG_COUNT];  
  struct cxn_ctx ctx[BACKLOG_COUNT];
};

int connection_queue_init(struct connection_queue *cxn_queue);
int connection_queue_add(struct connection_queue *cxn_queue,
						 int cxn_fd, const char *ip_addr, const int port);

struct cxn_ctx *connection_queue_get_tail(struct connection_queue *cxn_queue);
struct cxn_ctx *connection_queue_find(struct connection_queue *cxn_queue, int fd);

int connection_queue_pop(struct connection_queue *cxn_queue);
int connection_queue_destroy(struct connection_queue *cxn_queue);

#endif /* CXN_ARR_HEADER */
