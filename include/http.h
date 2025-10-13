#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define MAX_HEADERS 50

struct Header {
    char key[50];
    char value[256];
};

struct Request {
    char method[16];
    char path[256];
    char version[16];
    struct Header headers[MAX_HEADERS];
    int header_count;
    char *body;
};

struct Response {
    int status_code;
    char content_type[64];
    char body[1024];
};

// HTTP Functions
void parse_http_request(char *recv_buffer, struct Request *req);
void send_response(int client_socket, struct Response *res);

#endif

