#include "../include/http.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

void send_response(int client_socket, struct Response *res) {
    char buffer[2048];
    char *status_text;

    switch (res->status_code) {
        case 200: status_text = "OK"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }

    int len = snprintf(buffer, sizeof(buffer),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        res->status_code,
        status_text,
        res->content_type,
        strlen(res->body),
        res->body
    );

    send(client_socket, buffer, len, 0);
}


void parse_http_request(char *recv_buffer, struct Request *req) {
    char *saveptr_outer, *saveptr_inner;
    char *line = strtok_r(recv_buffer, "\n", &saveptr_outer);
    int line_no = 0;
    int in_headers = 1;

    req->header_count = 0;
    req->body = NULL;

    while (line != NULL) {
        line[strcspn(line, "\r")] = 0;

        if (strlen(line) == 0) {
            in_headers = 0;
            line = strtok_r(NULL, "", &saveptr_outer);
            if (line && strlen(line) > 0) {
                req->body = strdup(line);
            }
            break;
        }

        if (line_no == 0) {
            char *token = strtok_r(line, " ", &saveptr_inner);
            if (token) strncpy(req->method, token, sizeof(req->method));
            token = strtok_r(NULL, " ", &saveptr_inner);
            if (token) strncpy(req->path, token, sizeof(req->path));
            token = strtok_r(NULL, " ", &saveptr_inner);
            if (token) strncpy(req->version, token, sizeof(req->version));
        } else if (in_headers) {
            char *colon_pos = strchr(line, ':');
            if (colon_pos) {
                int index = req->header_count;
                size_t key_len = colon_pos - line;
                strncpy(req->headers[index].key, line, key_len);
                req->headers[index].key[key_len] = '\0';
                char *value = colon_pos + 1;
                while (*value == ' ') value++;
                strncpy(req->headers[index].value, value, sizeof(req->headers[index].value));
                req->header_count++;
            }
        }

        line_no++;
        line = strtok_r(NULL, "\n", &saveptr_outer);
    }
}

