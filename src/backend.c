#include "../include/backend.h"
#include "../include/logging.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

void init_backends(struct BackendPool *pool) {
    pool->count = 3;
    pool->current_index = 0;

    strcpy(pool->backends[0].host, "127.0.0.1");
    pool->backends[0].port = 9001;
    pool->backends[0].weight = 1;
    pool->backends[0].active_connections = 0;
    pool->backends[0].is_active = true;

    strcpy(pool->backends[1].host, "127.0.0.1");
    pool->backends[1].port = 9002;
    pool->backends[1].weight = 1;
    pool->backends[1].active_connections = 0;
    pool->backends[1].is_active = true;

    strcpy(pool->backends[2].host, "127.0.0.1");
    pool->backends[2].port = 9003;
    pool->backends[2].weight = 1;
    pool->backends[2].active_connections = 0;
    pool->backends[2].is_active = true;

    log_message(LOG_INFO, "Initialized %d backend servers", pool->count);
    
    for (int i = 0; i < pool->count; i++) {
        log_message(LOG_DEBUG, "Backend %d: %s:%d (active: %s, weight: %d)", 
                   i + 1, 
                   pool->backends[i].host, 
                   pool->backends[i].port,
                   pool->backends[i].is_active ? "true" : "false",
                   pool->backends[i].weight);
    }
}

struct Backend *get_next_backend(struct BackendPool *pool) {
    if (pool->count == 0) {
        log_error("backend", "no_backends_configured", "Backend pool is empty");
        return NULL;
    }

    int attempts = 0;

    while (attempts < pool->count) {
        struct Backend *backend = &pool->backends[pool->current_index];
        
        if (backend->is_active) {
            log_message(LOG_DEBUG, "Selected backend %s:%d (index: %d, connections: %d)", 
                       backend->host, backend->port, pool->current_index, backend->active_connections);
            pool->current_index = (pool->current_index + 1) % pool->count;
            return backend;
        }

        log_message(LOG_WARNING, "Backend %s:%d is inactive, skipping", 
                   backend->host, backend->port);
        
        pool->current_index = (pool->current_index + 1) % pool->count;
        attempts++;
    }

    log_error("backend", "no_active_backends", "No active backends available in pool");
    return NULL;
}

int forward_to_backend(struct Backend *backend, const char *request) {
    if (!backend || !backend->is_active) {
        log_error("backend", "invalid_backend", "Cannot forward to invalid or inactive backend");
        return -1;
    }

    log_message(LOG_DEBUG, "Forwarding request to backend %s:%d", 
               backend->host, backend->port);
    log_message(LOG_DEBUG, "Request content (first 200 chars): %.200s", request);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_error("backend", "socket_creation_failed", "Failed to create socket for backend connection");
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(backend->port);
    
    if (inet_pton(AF_INET, backend->host, &serv_addr.sin_addr) <= 0) {
        log_error("backend", "invalid_address", "Invalid backend address");
        close(sockfd);
        return -1;
    }

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log_error("backend", "connection_failed", "Failed to connect to backend server");
        close(sockfd);
        backend->is_active = false;  // Mark backend as inactive
        log_message(LOG_WARNING, "Marked backend %s:%d as inactive due to connection failure", 
                   backend->host, backend->port);
        return -1;
    }

    log_message(LOG_INFO, "Connected to backend %s:%d", backend->host, backend->port);

    size_t request_len = strlen(request);
    ssize_t sent_bytes = send(sockfd, request, request_len, 0);
    
    if (sent_bytes < 0) {
        log_error("backend", "send_failed", "Failed to send request to backend");
        close(sockfd);
        backend->is_active = false;
        log_message(LOG_WARNING, "Marked backend %s:%d as inactive due to send failure", 
                   backend->host, backend->port);
        return -1;
    }

    if (sent_bytes != request_len) {
        log_message(LOG_WARNING, "Partial send to backend %s:%d (%zd/%zu bytes)", 
                   backend->host, backend->port, sent_bytes, request_len);
    }

    log_message(LOG_DEBUG, "Sent %zd bytes to backend %s:%d", 
               sent_bytes, backend->host, backend->port);

    char response_buffer[4096];
    ssize_t received_bytes = recv(sockfd, response_buffer, sizeof(response_buffer) - 1, 0);
    
    if (received_bytes < 0) {
        log_error("backend", "receive_failed", "Failed to receive response from backend");
        close(sockfd);
        backend->is_active = false;
        log_message(LOG_WARNING, "Marked backend %s:%d as inactive due to receive failure", 
                   backend->host, backend->port);
        return -1;
    }

    if (received_bytes == 0) {
        log_message(LOG_WARNING, "Backend %s:%d closed connection unexpectedly", 
                   backend->host, backend->port);
        close(sockfd);
        return -1;
    }

    response_buffer[received_bytes] = '\0';
    log_message(LOG_DEBUG, "Received %zd bytes from backend %s:%d", 
               received_bytes, backend->host, backend->port);
    log_message(LOG_DEBUG, "Response (first 200 chars): %.200s", response_buffer);

    close(sockfd);
    log_message(LOG_INFO, "Successfully forwarded request to backend %s:%d", 
               backend->host, backend->port);
    
    return 0;
}