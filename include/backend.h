// include/backend.h
#ifndef BACKEND_H
#define BACKEND_H

#include <stdbool.h>

struct Backend {
    char host[50];
    int port;
    int active_connections;
    bool is_active;
};

struct BackendPool {
    struct Backend backends[10];
    int count;
    int current_index;
};

void init_backends(struct BackendPool *pool);
struct Backend *get_next_backend(struct BackendPool *pool);

#endif

