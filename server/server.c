#include "server.h"

static int server_loop(int server_fd, struct sockaddr_in *socket_addr);
static void * handle_connection(void *cxn_fd);
static char *get_request(int cxn_fd, size_t size);
static int parse_request(char *req_buf, struct req_info *req);
static int send_response(int cxn_fd, struct req_info *req);
static void send_ok_status(int cxn_fd, struct req_info *req);
static int print_dir_to_content_buf(char *path, struct content_buf *cb);
static int print_file_content_to_content_buf(char *path, struct content_buf *cb);
static void add_href(struct content_buf *cb, int path_len, const char *path,
					 const char *obj,
					 const char *name);

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
  struct connection_array cxn_arr = {
	.count = 0,
  };
#endif
  while (1) {
	int cxn_fd = accept(server_fd, (struct sockaddr *) socket_addr, (socklen_t *) &addr_len);  

	if (cxn_fd < 0) {
	  perror("failed accept connection");
	  return -1;
	}
	
	handle_connection(cxn_fd);
  }	

  return 0;
}

static int handle_connection(int cxn_fd)
{
  char *req_buf = get_request(cxn_fd, REQ_BUF_SIZE);
	
  struct req_info req = {
	.req_type = NULL,
	.path     = NULL,
	.html_ver = NULL,
  };

  int err = parse_request(req_buf, &req);

  if (err) {
	PRINT_ERR("failed to parse request\n");
	free(req_buf);
	close(cxn_fd);

	return -1;
  }  
	
  err = send_response(cxn_fd, &req);
	
  free(req_buf);	
  close(cxn_fd);

  return 0;
}

static void send_ok_status(int cxn_fd, struct req_info *req)
{
  dprintf(cxn_fd, "%s 200 OK\r\n", req->html_ver);
}

static void send_not_found_status(int cxn_fd, struct req_info *req)
{
  dprintf(cxn_fd, "%s 404 Not found\r\n", req->html_ver);
}
  
static int send_response(int cxn_fd, struct req_info *req)
{
  struct content_buf cb;
  content_buf_init(&cb);
  content_buf_add(&cb, "<!DOCTYPE html><html><body>");  

  struct stat path_stat;
  int err = stat(req->path, &path_stat);

  if (err != 0) {
	send_not_found_status(cxn_fd, req);
	content_buf_add(&cb, "<p>File/dir not found</p>");
  } else {
	send_ok_status(cxn_fd, req);
  }

  if (err != 0) {
	if (S_ISDIR(path_stat.st_mode)) {
	  print_dir_to_content_buf(req->path, &cb);
	} else {
	  print_file_content_to_content_buf(req->path, &cb);
	}
  }
  
  content_buf_add(&cb, "</body></html>");

  dprintf(cxn_fd,
		  "Content-Type: text/html\r\n"
		  "Content-Length: %d\r\n\r\n", cb.size);

  content_buf_print(&cb, cxn_fd);

  content_buf_destroy(&cb);
  
  return 0;
}

static int print_file_content_to_content_buf(char *path, struct content_buf *cb)
{
  int file_fd = open(path, O_RDONLY);

  char read_buf[READ_BUF_SIZE + 1];

  int read_size = 0;

  add_href(cb, path_len, path, "..", "[D] GO BACK");
  add_href(cb, path_len, path, ".",  "[D] CURRENT DIRECTORY");
  
  do {
	read_size = read(file_fd, read_buf, READ_BUF_SIZE);

	if (read < 0) {
	  break;
	}

	read_buf[READ_BUF_SIZE] = '\0';
	
	content_buf_add("<p>");
	content_buf_add("%s", read_buf);
	content_buf_add("</p>");	
  } while (read_size > 0);
  
  return 0;
}

static int print_dir_to_content_buf(char *path, struct content_buf *cb)
{
  chdir("/");

  int path_len = strlen(path);

  struct stat file_info;
  
  DIR *dir = opendir(path);

  if (dir == NULL) {
	return -1;
  }

  struct dirent *dir_entry = NULL;

  add_href(cb, path_len, path, "..", "[D] GO BACK");
  add_href(cb, path_len, path, ".",  "[D] CURRENT DIRECTORY");
  
  while ((dir_entry = readdir(dir)) != NULL) {
	if (*dir_entry->d_name == '.') {
	  continue;
	}
	add_href(cb, path_len, path, dir_entry->d_name, dir_entry->d_name);
  }

  closedir(dir);
  
  return 0;
}

static void add_href(struct content_buf *cb,
					 int path_len,
					 const char *path,
					 const char *obj,
					 const char *name)
{
  content_buf_add(cb, "<a href=\"http://%s:%d\n", LOCALHOST_IP_ADDR, DEFAULT_CXN_PORT);

  if (path[path_len - 1] == '/') {
	content_buf_add(cb, "%s%s", path, obj);
  } else {
	content_buf_add(cb, "%s/%s", path, obj);
  }

  content_buf_add(cb, "\">%s</a><br>\n", name);
}

static int parse_request(char *req_buf, struct req_info *req)
{
  if (!req_buf) {
	return -1;
  }

  char *save_ptr = NULL;
  
  req->req_type = strtok_r(req_buf, REQUEST_DELIM, &save_ptr);
  if (!req->req_type) {
	return -1;
  }
  
  req->path = strtok_r(NULL, REQUEST_DELIM, &save_ptr);
  if (!req->path) {
	return -1;
  }

  req->html_ver = strtok_r(NULL, REQUEST_DELIM, &save_ptr);  
  if (!req->html_ver) {
	return -1;
  }
  
  return 0;
}

static char *get_request(int cxn_fd, size_t size)
{
  char *req_buf = (char *) malloc(sizeof(char) * size);

  char read_buf[READ_BUF_SIZE];

  FILE *cxn_file = fdopen(cxn_fd, "r");

  char *req_str = req_buf;
  size_t str_size = size;
  
  for (;; req_str = read_buf, str_size = READ_BUF_SIZE) {
	char *err_str = fgets(req_str, str_size, cxn_file);

	if (err_str == NULL) {
	  free(req_buf);
	  req_buf = NULL;
	  break;
	}

	if (*err_str == '\r') {
	  break;
	}
  }

  return req_buf;
}
