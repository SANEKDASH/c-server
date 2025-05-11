#include <unistd.h>

#include "server/server.h"

int main(int argc, char **argv)
{
  const char *ip_addr = LOCALHOST_IP_ADDR;
  int port = DEFAULT_CXN_PORT;

  int opt;

  while ((opt = getopt(argc, argv, "a:p:")) != -1) {
	switch(opt) {
	case 'p': {
	  port = atoi(optarg);
	  if (port < 0) {
		PRINT_ERR("invalid port: %s\n", optarg);
	  }
	  
	  break;
	}
	case 'a': {
	  ip_addr = optarg; 
	  break;
	}
	default: {
	  PRINT_ERR("unknown flag: %s\n", optarg);
	  break;
	}
	}
  }
  
  run_server(ip_addr, port);
  
  return 0;
}
