#include "server.h"

static int server_loop(int server_fd, struct sockaddr_in *socket_addr);
static void * handle_connection(void *cxn_fd);

static int get_request  (struct cxn_ctx *cxn_ctx);
static int parse_request(struct cxn_ctx *cxn_ctx);
static int send_response(struct cxn_ctx *cxn_ctx);

static int get_requested_data(struct cxn_ctx *cxn_ctx);
static int get_dir_data(struct cxn_ctx *cxn_ctx);
static void create_dir_content(struct cxn_ctx *cxn_ctx);
static int create_file_content(struct cxn_ctx *cxn_ctx);
static void create_response_content(struct cxn_ctx *cxn_ctx);

static int send_not_found_response(struct cxn_ctx *cxn_ctx);

static void add_href(struct content_buf *cb, const char *path, const char *obj, const char *name);

#ifdef MULTITHREAD
static int handle_multithread_connection(struct connection_array *cxn_arr, void *cxn_fd_p);
#endif

int run_server(const char *ip, int port)
{
  PRINT_LOG("size = %ld\n", sizeof(struct dirent));
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
  
  err = inet_aton(LOCALHOST_IP_ADDR, &sock_addr.sin_addr);

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

  server_loop(socket_fd, &sock_addr);

  close(socket_fd);

  return 0;
}

static int server_loop(int server_fd, struct sockaddr_in *socket_addr)
{
  int addr_len = sizeof(*socket_addr);
 
#ifdef MULTITHREAD
  struct connection_array cxn_arr;
  cxn_arr.count = 0;
#endif

  while (1) {
	int cxn_fd = accept(server_fd, (struct sockaddr *) socket_addr, (socklen_t *) &addr_len);  

	PRINT_LOG("connection established: descriptor: %d\n", cxn_fd);
	
	if (cxn_fd < 0) {
	  perror("failed accept connection");
	  return -1;
	}

#ifdef MULTITHREAD
	handle_multithread_connection(&cxn_arr, cxn_fd);
#else   
	handle_connection(&cxn_fd);
#endif	
  }	

  return 0;
}

#ifdef MULTITHREAD
static int handle_multithread_connection(struct connection_array *cxn_arr, void *cxn_fd_p)
{
  	cxn_arr->fds[cxn_arr->count] = cxn_fd;
	
	pthread_create(&cxn_arr->threads[cxn_arr->count], NULL, handle_connection,
				   &cxn_arr->fds[cxn_arr->count]);
	
	cxn_arr->count++;

	if (cxn_arr->count >= BACKLOG_COUNT) {
	  PRINT_LOG("fds count - %d\n", cxn_arr->count);

	  for (int i = 0; i < cxn_arr->count; i++) {
		if (cxn_arr->fds[i] != -1) {		  
		  pthread_join(cxn_arr->threads[i], NULL);
		  cxn_arr->fds[i] = -1;		  
		}
	  }
	  
	  cxn_arr->count = 0;
	}
	
  return 0;
}
#endif

static void *handle_connection(void *cxn_fd_p)
{
  struct cxn_ctx cxn_ctx;

  int fd = *((int *) cxn_fd_p);
  
  cxn_ctx_init(&cxn_ctx, fd);
 
  int err = get_request(&cxn_ctx);
  if (err) {
	PRINT_ERR("getting request\n");
	cxn_ctx_destroy(&cxn_ctx);
	return NULL;
  }

  err = parse_request(&cxn_ctx);
  if (err) {
	PRINT_ERR("parsing request...\n");
	cxn_ctx_destroy(&cxn_ctx);
	return NULL;
  }

  err = get_requested_data(&cxn_ctx);
  if (err) {
    PRINT_ERR("getting requested data...\n");
	send_not_found_response(&cxn_ctx);
	cxn_ctx_destroy(&cxn_ctx);
	return NULL;
  }

  create_response_content(&cxn_ctx);

  err = send_response(&cxn_ctx);
  if (err) {
    PRINT_ERR("sending response\n");
	cxn_ctx_destroy(&cxn_ctx);
	return NULL;
  }

  cxn_ctx_destroy(&cxn_ctx);
  
  return NULL;
}

static int send_not_found_response(struct cxn_ctx *cxn_ctx)
{
  const char *not_found_msg = "<!DOCTYPE html><html><body><h1>404 Not Found</h1></body></html>";
  
  dprintf(cxn_ctx->cxn_fd,
		  "HTTP/1.1 404 Not Found\r\n"
		  "Content-Type: text/html\r\n"
		  "Content-Length: %ld\r\n\r\n"
		  "%s", strlen(not_found_msg), not_found_msg);
  return 0;
}

static int get_request(struct cxn_ctx *cxn_ctx)
{
  char read_buf[READ_BUF_SIZE];
  
  while (1) {
	char *err_str = fgets(read_buf, READ_BUF_SIZE, cxn_ctx->cxn_file);

	if (err_str == NULL) {	  
	  break;
	}

	if (*err_str == '\r') {
	  break;
	}
	
	content_buf_add(&cxn_ctx->req, "%s", read_buf);
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
	cur_node = cur_node->next;	
  }
  
  return 0;
}

static void create_response_content(struct cxn_ctx *cxn_ctx)
{
  assert(cxn_ctx->resp.size == 0);
  
  content_buf_add(&cxn_ctx->resp, "<!DOCTYPE html><html><body>\n");
  
  add_href(&cxn_ctx->resp, cxn_ctx->req_info.path, "..", "GO_BACK");

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
	  add_href(&(cxn_ctx->resp), cxn_ctx->req_info.path, cur_node->dnt.d_name, cur_node->dnt.d_name);
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
		  "Content-Length: %d\r\n\r\n", cxn_ctx->req_info.html_ver, cxn_ctx->resp.size);
  
  content_buf_print(&cxn_ctx->resp, cxn_ctx->cxn_fd);
  
  return 0;
}

static void add_href(struct content_buf *cb,
					 const char *path,
					 const char *obj,
					 const char *name)
{
  // slow
  int path_len = strlen(path);
  
  content_buf_add(cb, "<a href=\"http://%s:%d", LOCALHOST_IP_ADDR, DEFAULT_CXN_PORT);
  
  if (path[path_len - 1] == '/') {
	content_buf_add(cb, "%s%s", path, obj);
  } else {
	content_buf_add(cb, "%s/%s", path, obj);
  }

  content_buf_add(cb, "\">%s</a><br>\n", name);
}
