#include "../include/server.h"
#include "../include/backend.h"
#include<stdio.h>

int main() {
  struct BackendPool backend_pool;
  init_backends(&backend_pool);
  printf("Initialized %d backend servers for load balancing.\n", backend_pool.count);

  struct Backend *selected = get_next_backend(&backend_pool);
  printf("Selected backend → %s:%d\n", selected->host, selected->port);

    start_server();
    return 0;
}

