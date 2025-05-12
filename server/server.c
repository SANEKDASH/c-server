#include "server.h"

static int server_loop(int server_fd, struct sockaddr_in *socket_addr,
					   const char *ip_addr, const int port);
static void *handle_connection(void *cxn_ctx_p);

static int get_request  (struct cxn_ctx *cxn_ctx);
static int parse_request(struct cxn_ctx *cxn_ctx);
static int send_response(struct cxn_ctx *cxn_ctx);

static int get_requested_data(struct cxn_ctx *cxn_ctx);
static int get_dir_data(struct cxn_ctx *cxn_ctx);
static void create_dir_content(struct cxn_ctx *cxn_ctx);
static int create_file_content(struct cxn_ctx *cxn_ctx);
static void create_response_content(struct cxn_ctx *cxn_ctx);

static int send_not_found_response(struct cxn_ctx *cxn_ctx);

static void add_href(struct content_buf *cb, const char *ip_addr, const int port,
					 const char *path, const char *obj, const char *name);

#ifdef MULTITHREAD
static int handle_multithread_connection(struct connection_array *cxn_arr, int cxn_fd,
										 const char *ip_addr, const int port);
#endif

int run_server(const char *ip, int port)
{
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_fd < 0) {
	perror("failed to get socket fd");
	return -1;
  }
  
  struct sockaddr_in sock_addr;

  sock_addr.sin_family      = AF_INET;
  sock_addr.sin_addr.s_addr = INADDR_ANY;
  sock_addr.sin_port        = htons(port);

  int err = 0;
  
  err = inet_aton(ip, &sock_addr.sin_addr);

  if (!err) {
    PRINT_ERR("failed to convert addr");
	return -1;
  }

  err = bind(socket_fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr));

  if (err < 0) {
	perror("failed to bind socket");
	return -1;
  }

  if(listen(socket_fd, BACKLOG_COUNT) < 0) {
	perror("listen failed");
	close(socket_fd);
	return -1;
  }

  PRINT_LOG("server is running on %s:%d\n", ip, port);

  server_loop(socket_fd, &sock_addr, ip, port);

  close(socket_fd);

  return 0;
}

static int server_loop(int server_fd, struct sockaddr_in *socket_addr,
					   const char *ip_addr, const int port)
{
  int addr_len = sizeof(*socket_addr);

  int epollfd = epoll_create1(0);    
  if (epollfd < 0) {
	perror("failed to create epoll instance");
	return -1;
  }  

  struct epoll_event cxn_events[BACKLOG_COUNT];
  
  struct connection_queue cxn_queue;
  connection_queue_init(&cxn_queue);

  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = server_fd;

  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
	perror("failed to add server descriptor to epoll instance");
	close(epollfd);
	return -1;
  }  
  
  while (1) {
	int events_count = epoll_wait(epollfd, cxn_events, sizeof(cxn_events), -1);

	if (events_count < 0) {	  
	  perror("failed epoll wait");
	  connection_queue_destroy(&cxn_queue);
	  close(epollfd);
	  return -1;
	}

	for (int i = 0; i < events_count; i++) {
	  if (cxn_events[i].data.fd == server_fd) {			
		int cxn_fd = accept(server_fd, (struct sockaddr *) socket_addr, (socklen_t *) &addr_len);  

		if (cxn_fd < 0) {
		  perror("failed accept connection");
		  return -1;
		}

		fcntl(cxn_fd, F_SETFL, O_NONBLOCK);
		event.events  = EPOLLIN | EPOLLET;
		event.data.fd = cxn_fd;

		if (epoll_ctl(epollfd, EPOLL_CTL_ADD, cxn_fd, &event) < 0) {
		  perror("failed to add connection descriptor to epoll instance");
		  return -1;
		}

	    while (connection_queue_add(&cxn_queue, cxn_fd, ip_addr, port) !=  0) {
		  connection_queue_pop(&cxn_queue);
		}

		PRINT_LOG("connection established: descriptor: %d\n", cxn_fd);
	  } else {
		struct cxn_ctx *cxn_ctx = connection_queue_find(&cxn_queue, cxn_events[i].data.fd);

		if (cxn_ctx == NULL) {
		  PRINT_ERR("failed to find context corresponding for discriptor %d\n", cxn_events[i].data.fd);
		  continue;
		}

		handle_connection(cxn_ctx);

		epoll_ctl(epollfd, EPOLL_CTL_DEL, cxn_ctx->cxn_fd, NULL);		
	  }
	}	
  }
  
  connection_queue_destroy(&cxn_queue);

  return 0;
}

static void *handle_connection(void *cxn_ctx_p)
{
  struct cxn_ctx *cxn_ctx = (struct cxn_ctx *) cxn_ctx_p;
  
  int err = get_request(cxn_ctx);
  if (err) {
	PRINT_ERR("getting request\n");
	send_not_found_response(cxn_ctx);
	return NULL;
  }

  err = parse_request(cxn_ctx);
  if (err) {
	PRINT_ERR("parsing request...\n");
	send_not_found_response(cxn_ctx);
	return NULL;
  }

  err = get_requested_data(cxn_ctx);
  if (err) {
    PRINT_ERR("getting requested data...\n");
	send_not_found_response(cxn_ctx);
	return NULL;
  }

  create_response_content(cxn_ctx);

  err = send_response(cxn_ctx);
  if (err) {
    PRINT_ERR("sending response\n");
	return NULL;
  }
  
  return NULL;
}

static int send_not_found_response(struct cxn_ctx *cxn_ctx)
{
  const char *not_found_msg = "<!DOCTYPE html><html><body><h1>404 Not Found</h1></body></html>";
  
  dprintf(cxn_ctx->cxn_fd,
		  "HTTP/1.1 404 Not Found\r\n"
		  "Content-Type: text/html\r\n"
		  "Content-Length: %ld\r\n"
		  "Connection: close\r\n\r\n"
		  "%s", strlen(not_found_msg), not_found_msg);
  return 0;
}

static int get_request(struct cxn_ctx *cxn_ctx)
{
  char read_buf[READ_BUF_SIZE + 1];

  int read_size = 0;
  
  while (1) {
	PRINT_LOG("reading from %d descriptor\n", cxn_ctx->cxn_fd);
	read_size = read(cxn_ctx->cxn_fd, read_buf, READ_BUF_SIZE);
	PRINT_LOG("readed %d bytes\n", read_size);
	
	if (read_size <= 0) {
	  break;
	}
	
	read_buf[read_size] = '\0';

	content_buf_add(&cxn_ctx->req, "%s", read_buf);

	if (read_buf[read_size - 1] == '\r') {
	  break;
	}
  }
  
  return 0;
}

static int parse_request(struct cxn_ctx *cxn_ctx)
{
  char *save_ptr = NULL;
  char *request = cxn_ctx->req.buf;
  
  cxn_ctx->req_info.req_type = strtok_r(request, REQUEST_DELIM, &save_ptr);
  if (!cxn_ctx->req_info.req_type) {
	return -1;
  }
  
  cxn_ctx->req_info.path = strtok_r(NULL, REQUEST_DELIM, &save_ptr);
  if (!cxn_ctx->req_info.path) {
	return -1;
  }

  /*
   * Actually it's not.
   * But now we assume it is.
   */

  cxn_ctx->req_info.html_ver = strtok_r(NULL, REQUEST_DELIM, &save_ptr);  
  if (!cxn_ctx->req_info.html_ver) {
	return -1;
  }
  
  return 0;
}

static int get_requested_data(struct cxn_ctx *cxn_ctx)
{
  struct stat path_stat;
  PRINT_LOG("path - %s\n", cxn_ctx->req_info.path);
  int err = stat(cxn_ctx->req_info.path, &path_stat);

  if (err) {
	perror("failed stat");
	return -1;
  }
  
  if (S_ISDIR(path_stat.st_mode)) {
	err = get_dir_data(cxn_ctx);

	if (err < 0) {
	  return -1;
	}
  } else {
    cxn_ctx->file_fd = open(cxn_ctx->req_info.path, O_RDONLY);

	if (cxn_ctx->file_fd < 0) { 
	  return -1;
	}
  }
  
  return 0;
}

static int get_dir_data(struct cxn_ctx *cxn_ctx)
{  
  cxn_ctx->dir = opendir(cxn_ctx->req_info.path);
  
  if (cxn_ctx->dir == NULL) {
	perror("failed dir");
	return -1;
  }

  struct dirent *dir_entry = NULL;

  struct dirent_node *cur_node = cxn_ctx->dnt_node;

  if (cur_node == NULL) {
	if ((dir_entry = readdir(cxn_ctx->dir)) != NULL) {
	  cxn_ctx->dnt_node = dirent_node_alloc(dir_entry);
	  cur_node = cxn_ctx->dnt_node;
	}
  }
  
  while ((dir_entry = readdir(cxn_ctx->dir)) != NULL) {	
	cur_node->next = dirent_node_alloc(dir_entry);	
	if (cur_node->next == NULL) {
	  dirent_node_destroy(cxn_ctx->dnt_node);
	  cxn_ctx->dnt_node = NULL;	  
	  return -1;
	}
	
	cur_node = cur_node->next;	
  }
  
  return 0;
}

static void create_response_content(struct cxn_ctx *cxn_ctx)
{
  assert(cxn_ctx->resp.size == 0);
  
  content_buf_add(&cxn_ctx->resp, "<!DOCTYPE html><html><body>\n");
  
  add_href(&cxn_ctx->resp, cxn_ctx->ip_addr, cxn_ctx->port, cxn_ctx->req_info.path,
		   "..", "GO_BACK");

  if (cxn_ctx->file_fd < 0) {
	create_dir_content(cxn_ctx);
  } else {
    create_file_content(cxn_ctx);
  }

  content_buf_add(&cxn_ctx->resp, "</body></html>");
}

static void create_dir_content(struct cxn_ctx *cxn_ctx)
{
  struct dirent_node *cur_node = cxn_ctx->dnt_node;

  while (cur_node != NULL) {	
	if (*(cur_node->dnt.d_name) != '.') {
	  add_href(&(cxn_ctx->resp), cxn_ctx->ip_addr, cxn_ctx->port, cxn_ctx->req_info.path,
			   cur_node->dnt.d_name, cur_node->dnt.d_name);
	}

	cur_node = cur_node->next;
  }
}

static int create_file_content(struct cxn_ctx *cxn_ctx)
{
  if (cxn_ctx->cxn_fd < 0) {
	return -1;
  }
  
  char read_buf[READ_BUF_SIZE + 1];
  int read_size = 0;

  content_buf_add(&cxn_ctx->resp, "<pre>");

  do {
	read_size = read(cxn_ctx->file_fd, read_buf, READ_BUF_SIZE);

	if (read < 0) {
	  break;
	}
		
	read_buf[read_size] = '\0';
	
	content_buf_add(&cxn_ctx->resp, "%s", read_buf);
  } while (read_size > 0);

  content_buf_add(&cxn_ctx->resp, "</pre>");	

  return 0;
}
static int send_response(struct cxn_ctx *cxn_ctx)
{ 
  dprintf(cxn_ctx->cxn_fd,
		  "%s 200 OK\r\n"
		  "Content-Type: text/html\r\n"
		  "Content-Length: %d\r\n"
		  "Connection: close\r\n\r\n", cxn_ctx->req_info.html_ver, cxn_ctx->resp.size);
  
  content_buf_print(&cxn_ctx->resp, cxn_ctx->cxn_fd);
  
  return 0;
}

static void add_href(struct content_buf *cb,
					 const char *ip_addr,
					 const int port,
					 const char *path,
					 const char *obj,
					 const char *name)
{
  int path_len = strlen(path);
  
  content_buf_add(cb, "<a href=\"http://%s:%d", ip_addr, port);
  
  if (path[path_len - 1] == '/') {
	content_buf_add(cb, "%s%s", path, obj);
  } else {
	content_buf_add(cb, "%s/%s", path, obj);
  }

  content_buf_add(cb, "\">%s</a><br>\n", name);
}
