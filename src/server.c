#include "../include/server.h"
#include "../include/http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

void handle_client(int client_socket, char *recv_buffer) {
    struct Request req;
    parse_http_request(recv_buffer, &req);

    printf("Method: %s\n", req.method);
    printf("Path: %s\n", req.path);
    printf("Version: %s\n", req.version);

    struct Response res;
    res.status_code = 200;
    strcpy(res.content_type, "application/json");
    snprintf(res.body, sizeof(res.body),
             "{\"message\": \"Hello from Modular HTTP Server\", \"path\": \"%s\"}",
             req.path);

    send_response(client_socket, &res);

    free(req.body);
    close(client_socket);
}


void start_server() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    char recv_buffer[BUFFER_SIZE];
    socklen_t c = sizeof(struct sockaddr_in);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_socket);
        exit(FAILURE);
    }

    if (listen(server_socket, 5) == -1) {
        perror("Listen failed");
        close(server_socket);
        exit(FAILURE);
    }

    printf("Server listening on port %d...\n", SERVER_PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &c);
        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        }

        ssize_t bytes_recv = recv(client_socket, recv_buffer, BUFFER_SIZE - 1, 0);
        if (bytes_recv <= 0) {
            perror("Receive failed");
            close(client_socket);
            continue;
        }

        recv_buffer[bytes_recv] = '\0';
        handle_client(client_socket, recv_buffer);
    }
}

