#include "server/server.h"

int main()
{  	  
  run_server(LOCALHOST_IP_ADDR, DEFAULT_CXN_PORT);
  
  return 0;
}
