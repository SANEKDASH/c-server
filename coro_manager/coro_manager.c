#include "coro_manager.h"

int manage_server_coros(struct cxn_arr *cxn_arr)
{
  int epolfd = -1;
  epolfd = epoll_create1(0);

  if (epolfd < 0) {
	perror("failed to create epoll instance");
	return -1;
  }
  
  return 0;
}
