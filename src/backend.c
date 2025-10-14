// src/backend.c
#include "../include/backend.h"
#include <stdio.h>
#include <string.h>

// Initialize static list of backends
void init_backends(struct BackendPool *pool) {
    pool->count = 3;
    pool->current_index = 0;

    strcpy(pool->backends[0].host, "127.0.0.1");
    pool->backends[0].port = 9001;
    pool->backends[0].is_active = true;

    strcpy(pool->backends[1].host, "127.0.0.1");
    pool->backends[1].port = 9002;
    pool->backends[1].is_active = true;

    strcpy(pool->backends[2].host, "127.0.0.1");
    pool->backends[2].port = 9003;
    pool->backends[2].is_active = true;
}

// Round-robin selection
struct Backend *get_next_backend(struct BackendPool *pool) {
    if (pool->count == 0) return NULL;

    struct Backend *backend = &pool->backends[pool->current_index];
    pool->current_index = (pool->current_index + 1) % pool->count;
    return backend;
}

