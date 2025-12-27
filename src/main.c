#include "../include/server.h"
#include "../include/backend.h"
#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

struct BackendPool backend_pool;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        log_message(LOG_INFO, "Received shutdown signal, shutting down gracefully...");
        exit(0);
    }
}

int main() {
    init_logging();
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    log_message(LOG_INFO, "=========================================");
    log_message(LOG_INFO, "Load Balancer Server Starting");
    log_message(LOG_INFO, "=========================================");
    
    init_backends(&backend_pool);
    
    log_message(LOG_INFO, "Successfully initialized backend pool with %d servers", backend_pool.count);
    
    for (int i = 0; i < backend_pool.count; i++) {
        log_message(LOG_INFO, "Backend %d: %s:%d (Weight: %d, Active: %s)", 
                   i + 1, 
                   backend_pool.backends[i].host, 
                   backend_pool.backends[i].port,
                   backend_pool.backends[i].weight,
                   backend_pool.backends[i].is_active ? "true" : "false");
    }
    
    struct Backend *selected = get_next_backend(&backend_pool);
    if (selected) {
        log_message(LOG_INFO, "Initial backend selection test: %s:%d", 
                   selected->host, selected->port);
        printf("Initialized %d backend servers for load balancing.\n", backend_pool.count);
        printf("Selected backend → %s:%d\n", selected->host, selected->port);
    } else {
        log_error("main", "backend_selection_failed", "No available backends for initial selection");
        printf("Warning: No available backends for initial selection\n");
    }
    
    log_message(LOG_INFO, "Server configuration loaded");
    
    pthread_t health_check_tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    if (pthread_create(&health_check_tid, &attr, health_check_thread, &backend_pool) != 0) {
        log_error("main", "health_check_thread_failed", "Failed to create health check thread");
        printf("Warning: Failed to start health check thread\n");
    } else {
        log_message(LOG_INFO, "Health check thread started successfully");
    }
    pthread_attr_destroy(&attr);
    
    log_message(LOG_INFO, "Starting server...");
    
    start_server();
    
    log_message(LOG_INFO, "Server has stopped");
    
    log_message(LOG_INFO, "=========================================");
    log_message(LOG_INFO, "Load Balancer Server Shutdown Complete");
    log_message(LOG_INFO, "=========================================");
    
    cleanup_logging();
    
    return EXIT_SUCCESS;
}