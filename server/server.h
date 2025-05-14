#ifndef SIMPLE_SYNC_SERVER_HEADER
#define SIMPLE_SYNC_SERVER_HEADER

#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <assert.h>

#include <sys/epoll.h>
#include "../common.h"
#include "../cxn_queue/cxn_queue.h"
#include "server.h"
#include "../cxn_ctx/cxn_ctx.h"
#include "../dirent_node/dnt_node.h"
#include "../my_coro_lib/my_coro.h"

struct server_routines {
  struct my_coro get_request;
  struct my_coro parse_request;
  struct my_coro get_requested_data;
  struct my_coro create_response_content;
  struct my_coro send_response;
};

static const uint16_t DEFAULT_CXN_PORT = 8080;

static const char *LOCALHOST_IP_ADDR = "127.0.0.1";

static const char *REQUEST_DELIM = " \n\r";

static const size_t REQ_BUF_SIZE = 256;

static const int OK_STATUS        = 200;
static const int NOT_FOUND_STATUS = 404;

#define READ_BUF_SIZE 256
#define DIR_NAME_LEN 128
#define RESPONSE_BUF_SIZE 256

int run_server(const char *ip, int port);

#endif /* SIMPLE_SYNC_SERVER_HEADER */
