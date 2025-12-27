#ifndef BACKEND_H
#define BACKEND_H

#include <stdbool.h>
#include <sys/types.h>

struct Backend {
    char host[50];
    int port;
    int weight;
    int active_connections;
    bool is_active;   // Backend health/availability status (set by health checks and request failures)
};

struct BackendPool {
    struct Backend backends[10];
    int count;
    int current_index;
    int current_weights[10];  // Current weights for weighted round-robin
};

void init_backends(struct BackendPool *pool, const char *config_file);
struct Backend *get_next_backend(struct BackendPool *pool);
// Returns response buffer (caller must free) and size via output parameters
// Returns 0 on success, -1 on failure
int forward_to_backend(struct Backend *backend, const char *request, 
                       char **response_buffer, ssize_t *response_size);
// Health checking functions
int health_check_backend(struct Backend *backend);
void* health_check_thread(void* arg);

#endif
