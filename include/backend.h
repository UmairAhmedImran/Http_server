#ifndef BACKEND_H
#define BACKEND_H

#include <stdbool.h>

struct Backend {
    char host[50];
    int port;
    int weight;
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
int forward_to_backend(struct Backend *backend, const char *request);
int check_backend_health(struct Backend *backend);
void mark_backend_inactive(struct Backend *backend);
void mark_backend_active(struct Backend *backend);
int connect_to_backend(struct Backend *backend);

#endif