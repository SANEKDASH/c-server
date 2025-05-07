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

#include "server.h"
#include "../cxn_ctx/cxn_ctx.h"
#include "../dirent_node/dnt_node.h"

#ifdef LOG_ON
#define PRINT_LOG(...) printf("[LOG]: " __VA_ARGS__)
#else
#define PRINT_LOG(...) 
#endif

#define PRINT_ERR(...) printf("[ERROR]: " __VA_ARGS__)

static const uint16_t DEFAULT_CXN_PORT = 8080;

static const char *LOCALHOST_IP_ADDR = "127.0.0.1";

static const char *REQUEST_DELIM = " \n\r";

#ifdef MULTITHREAD
#define BACKLOG_COUNT 16
struct connection_array {
  int count;
  pthread_t threads[BACKLOG_COUNT];  
  int fds[BACKLOG_COUNT];
};
#else
#define BACKLOG_COUNT 1
#endif

static const size_t REQ_BUF_SIZE = 256;

static const int OK_STATUS        = 200;
static const int NOT_FOUND_STATUS = 404;

#define READ_BUF_SIZE 256
#define DIR_NAME_LEN 128
#define RESPONSE_BUF_SIZE 256

int run_server(const char *ip, int port);

#endif /* SIMPLE_SYNC_SERVER_HEADER */
