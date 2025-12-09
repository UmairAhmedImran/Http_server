#include "../include/server.h"
#include "../include/http.h"
#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void handle_client(int client_socket, char *recv_buffer, struct sockaddr_in client_addr) {
    struct Request req;
    parse_http_request(recv_buffer, &req);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    log_message(LOG_INFO, "Received request from %s:%d", client_ip, ntohs(client_addr.sin_port));
    log_message(LOG_DEBUG, "Request: %s %s %s", req.method, req.path, req.version);

    struct Backend *selected_backend = get_next_backend(&backend_pool);
    
    if (!selected_backend) {
        log_error("server", "no_backend_available", "No backend servers available");
        
        struct Response res;
        res.status_code = 503;
        strcpy(res.content_type, "application/json");
        snprintf(res.body, sizeof(res.body),
                 "{\"error\": \"Service Unavailable\", \"message\": \"No backend servers available\"}");
        
        send_response(client_socket, &res);
        log_request(req.method, req.path, "none", 503);
        
        free(req.body);
        close(client_socket);
        return;
    }

    log_message(LOG_INFO, "Selected backend: %s:%d for request %s %s", 
                selected_backend->host, selected_backend->port, req.method, req.path);

    int forward_result = forward_to_backend(selected_backend, recv_buffer);
    
    // Create backend string with port for logging
    char backend_with_port[100];
    snprintf(backend_with_port, sizeof(backend_with_port), "%s:%d", 
             selected_backend->host, selected_backend->port);
    
    if (forward_result == 0) {
        struct Response res;
        res.status_code = 200;
        strcpy(res.content_type, "application/json");
        snprintf(res.body, sizeof(res.body),
                 "{\"message\": \"Request forwarded successfully\", \"backend\": \"%s:%d\", \"path\": \"%s\"}",
                 selected_backend->host, selected_backend->port, req.path);
        
        send_response(client_socket, &res);
        log_request(req.method, req.path, backend_with_port, 200);
    } else {
        log_error("server", "forward_failed", "Failed to forward request to backend");
        
        struct Response res;
        res.status_code = 502;
        strcpy(res.content_type, "application/json");
        snprintf(res.body, sizeof(res.body),
                 "{\"error\": \"Bad Gateway\", \"message\": \"Failed to forward to backend server\"}");
        
        send_response(client_socket, &res);
        log_request(req.method, req.path, backend_with_port, 502);
    }

    free(req.body);
    close(client_socket);
    log_message(LOG_DEBUG, "Client connection closed");
}

int start_server() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    char recv_buffer[BUFFER_SIZE];
    socklen_t c = sizeof(struct sockaddr_in);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        log_error("server", "socket_creation_failed", "Failed to create server socket");
        return FAILURE;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_error("server", "setsockopt_failed", "Failed to set socket options");
        close(server_socket);
        return FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        log_error("server", "bind_failed", "Failed to bind socket to port");
        close(server_socket);
        return FAILURE;
    }

    if (listen(server_socket, 5) == -1) {
        log_error("server", "listen_failed", "Failed to listen on socket");
        close(server_socket);
        return FAILURE;
    }

    log_message(LOG_INFO, "Server listening on port %d", SERVER_PORT);
    log_message(LOG_INFO, "Server ready to accept connections");

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &c);
        if (client_socket == -1) {
            log_error("server", "accept_failed", "Failed to accept client connection");
            continue;
        }

        ssize_t bytes_recv = recv(client_socket, recv_buffer, BUFFER_SIZE - 1, 0);
        if (bytes_recv <= 0) {
            if (bytes_recv == 0) {
                log_message(LOG_WARNING, "Client disconnected before sending data");
            } else {
                log_error("server", "receive_failed", "Error receiving data from client");
            }
            close(client_socket);
            continue;
        }

        recv_buffer[bytes_recv] = '\0';
        
        pid_t pid = fork();
        if (pid == 0) {
            close(server_socket);
            handle_client(client_socket, recv_buffer, client_addr);
            exit(0);
        } else if (pid < 0) {
            log_error("server", "fork_failed", "Failed to fork process for client handling");
            close(client_socket);
            continue;
        } else {
            close(client_socket);
        }
    }

    close(server_socket);
    return SUCCESS;
}