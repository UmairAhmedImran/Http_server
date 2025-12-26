#ifndef BACKEND_H
#define BACKEND_H

#include <stdbool.h>
#include <sys/types.h>

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
// Returns response buffer (caller must free) and size via output parameters
// Returns 0 on success, -1 on failure
int forward_to_backend(struct Backend *backend, const char *request, 
                       char **response_buffer, ssize_t *response_size);

#endif
