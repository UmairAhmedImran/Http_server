#include "../include/backend.h"
#include "../include/logging.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

void init_backends(struct BackendPool *pool) {
    // Adding values to the BackendPool struct
    pool->count = 3;
    pool->current_index = 0;

    strcpy(pool->backends[0].host, "127.0.0.1");
    pool->backends[0].port = 9001;
    pool->backends[0].weight = 3;  // Higher weight = more requests
    pool->backends[0].active_connections = 0;
    pool->backends[0].is_active = true;
    pool->current_weights[0] = pool->backends[0].weight;  // Initialize current weight

    strcpy(pool->backends[1].host, "127.0.0.1");
    pool->backends[1].port = 9002;
    pool->backends[1].weight = 2;  // Medium weight
    pool->backends[1].active_connections = 0;
    pool->backends[1].is_active = true;
    pool->current_weights[1] = pool->backends[1].weight;

    strcpy(pool->backends[2].host, "127.0.0.1");
    pool->backends[2].port = 9003;
    pool->backends[2].weight = 1;  // Lower weight
    pool->backends[2].active_connections = 0;
    pool->backends[2].is_active = true;
    pool->current_weights[2] = pool->backends[2].weight;

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

    // Calculate total weight of all active backends
    int total_weight = 0;
    for (int i = 0; i < pool->count; i++) {
        if (pool->backends[i].is_active) {
            total_weight += pool->backends[i].weight;
        }
    }

    if (total_weight == 0) {
        log_error("backend", "no_active_backends", "No active backends available in pool");
        return NULL;
    }

    // Weighted Round-Robin Algorithm:
    // 1. Find backend with highest current weight
    // 2. Subtract total weight from selected backend's current weight
    // 3. Add each backend's weight to its current weight
    // This ensures proportional distribution based on weights

    int max_weight_index = -1;
    int max_weight = -1;

    // Find backend with highest current weight (only active backends)
    for (int i = 0; i < pool->count; i++) {
        if (pool->backends[i].is_active && pool->current_weights[i] > max_weight) {
            max_weight = pool->current_weights[i];
            max_weight_index = i;
        }
    }

    if (max_weight_index == -1) {
        log_error("backend", "no_active_backends", "No active backends available in pool");
        return NULL;
    }

    // Update weights: subtract total weight from selected backend
    pool->current_weights[max_weight_index] -= total_weight;

    // Add each backend's weight to its current weight
    for (int i = 0; i < pool->count; i++) {
        if (pool->backends[i].is_active) {
            pool->current_weights[i] += pool->backends[i].weight;
        }
    }

    struct Backend *selected = &pool->backends[max_weight_index];
    log_message(LOG_DEBUG, "Selected backend %s:%d (index: %d, weight: %d, current_weight: %d)", 
               selected->host, selected->port, max_weight_index, 
               selected->weight, pool->current_weights[max_weight_index]);

    return selected;
}

int forward_to_backend(struct Backend *backend, const char *request, 
                       char **response_buffer, ssize_t *response_size) {
    if (!backend || !backend->is_active) {
        log_error("backend", "invalid_backend", "Cannot forward to invalid or inactive backend");
        return -1;
    }

    if (!response_buffer || !response_size) {
        log_error("backend", "invalid_parameters", "Response buffer or size pointer is NULL");
        return -1;
    }

    // Initialize output parameters
    *response_buffer = NULL;
    *response_size = 0;

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
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Failed to connect to backend server: %s (errno: %d)", 
                 strerror(errno), errno);
        log_error("backend", "connection_failed", err_msg);
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

    // Allocate buffer for response (start with 8KB, can grow if needed)
    size_t buffer_size = 8192;
    char *temp_buffer = malloc(buffer_size);
    if (!temp_buffer) {
        log_error("backend", "malloc_failed", "Failed to allocate memory for response buffer");
        close(sockfd);
        return -1;
    }

    ssize_t total_received = 0;
    ssize_t received_bytes;

    // Read response in chunks until connection closes or error
    // For HTTP/1.0, the server typically sends the full response and closes the connection
    // For HTTP/1.1, we need to check Content-Length or Connection: close
    // We'll read until recv returns 0 (connection closed) or we get an error
    while (1) {
        // Check if we need more space
        if (total_received >= (ssize_t)(buffer_size - 1)) {
            buffer_size *= 2;
            char *new_buffer = realloc(temp_buffer, buffer_size);
            if (!new_buffer) {
                log_error("backend", "realloc_failed", "Failed to reallocate response buffer");
                free(temp_buffer);
                close(sockfd);
                return -1;
            }
            temp_buffer = new_buffer;
        }

        received_bytes = recv(sockfd, temp_buffer + total_received, 
                             buffer_size - total_received - 1, 0);
        
        if (received_bytes < 0) {
            // Check if it's a timeout or actual error
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout - for HTTP/1.0, if we've received some data, assume it's complete
                // The server might have closed the connection but we didn't see the 0 return
                if (total_received > 0) {
                    log_message(LOG_DEBUG, "Receive timeout after %zd bytes, assuming response is complete", 
                               total_received);
                    break;
                }
                // No data received and timeout - this is an error
                char err_msg[256];
                snprintf(err_msg, sizeof(err_msg), "Timeout waiting for response from backend: %s (errno: %d)", 
                         strerror(errno), errno);
                log_error("backend", "receive_timeout", err_msg);
                free(temp_buffer);
                close(sockfd);
                backend->is_active = false;
                log_message(LOG_WARNING, "Marked backend %s:%d as inactive due to receive timeout", 
                           backend->host, backend->port);
                return -1;
            }
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "Failed to receive response from backend: %s (errno: %d)", 
                     strerror(errno), errno);
            log_error("backend", "receive_failed", err_msg);
            free(temp_buffer);
            close(sockfd);
            backend->is_active = false;
            log_message(LOG_WARNING, "Marked backend %s:%d as inactive due to receive failure", 
                       backend->host, backend->port);
            return -1;
        }

        if (received_bytes == 0) {
            // Connection closed by backend - we've received all data
            log_message(LOG_DEBUG, "Backend closed connection after sending %zd bytes", total_received);
            break;
        }

        total_received += received_bytes;
    }

    if (total_received == 0) {
        log_message(LOG_WARNING, "Backend %s:%d closed connection without sending data", 
                   backend->host, backend->port);
        free(temp_buffer);
        close(sockfd);
        return -1;
    }

    // Null-terminate for logging (but keep binary data intact for HTTP response)
    temp_buffer[total_received] = '\0';
    log_message(LOG_DEBUG, "Received %zd bytes from backend %s:%d", 
               total_received, backend->host, backend->port);
    log_message(LOG_DEBUG, "Response (first 200 chars): %.200s", temp_buffer);

    close(sockfd);
    log_message(LOG_INFO, "Successfully forwarded request to backend %s:%d", 
               backend->host, backend->port);
    
    // Return the response buffer and size
    *response_buffer = temp_buffer;
    *response_size = total_received;
    
    return 0;
}
