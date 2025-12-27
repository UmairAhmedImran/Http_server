#include "../include/http.h"
#include "../include/backend.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <strings.h>  // for strcasecmp

void send_response(int client_socket, struct Response *res) {
    char buffer[2048];
    char *status_text;

    switch (res->status_code) {
        case 200: status_text = "OK"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 502: status_text = "Bad Gateway"; break;
        case 503: status_text = "Service Unavailable"; break;
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

// Helper function to find a header by name (case-insensitive)
static struct Header* find_header(struct Request *req, const char *name) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, name) == 0) {
            return &req->headers[i];
        }
    }
    return NULL;
}

// Modify HTTP request to add proxy headers (X-Forwarded-For, X-Real-IP, Host)
// Returns a newly allocated string that must be freed by the caller
char* modify_request_for_proxy(const char *original_request, 
                                struct Request *req,
                                const char *client_ip,
                                struct Backend *backend) {
    if (!original_request || !req || !client_ip || !backend) {
        return NULL;
    }

    // Calculate size needed for modified request
    // Original request size + new headers (X-Forwarded-For, X-Real-IP, Host update)
    size_t original_len = strlen(original_request);
    size_t buffer_size = original_len + 512; // Extra space for new headers
    char *modified = malloc(buffer_size);
    if (!modified) {
        return NULL;
    }

    // Find existing headers
    struct Header *x_forwarded_for = find_header(req, "X-Forwarded-For");
    struct Header *x_real_ip = find_header(req, "X-Real-IP");

    // Build the new request
    char *ptr = modified;
    size_t remaining = buffer_size;

    // Copy request line (first line)
    const char *request_line_end = strstr(original_request, "\r\n");
    if (!request_line_end) {
        request_line_end = strstr(original_request, "\n");
    }
    if (request_line_end) {
        size_t line_len = request_line_end - original_request;
        if (line_len < remaining) {
            strncpy(ptr, original_request, line_len);
            ptr += line_len;
            remaining -= line_len;
            *ptr++ = '\r';
            *ptr++ = '\n';
            remaining -= 2;
        }
    } else {
        // Fallback: copy first line manually
        const char *first_newline = strchr(original_request, '\n');
        if (first_newline) {
            size_t line_len = first_newline - original_request;
            if (line_len < remaining) {
                strncpy(ptr, original_request, line_len);
                ptr += line_len;
                remaining -= line_len;
                *ptr++ = '\r';
                *ptr++ = '\n';
                remaining -= 2;
            }
        }
    }

    // Add/Update Host header
    char host_value[100];
    snprintf(host_value, sizeof(host_value), "%s:%d", backend->host, backend->port);
    int host_written = snprintf(ptr, remaining, "Host: %s\r\n", host_value);
    if (host_written > 0 && host_written < (int)remaining) {
        ptr += host_written;
        remaining -= host_written;
    }

    // Add/Update X-Forwarded-For header
    if (x_forwarded_for) {
        // Append to existing chain
        int xff_written = snprintf(ptr, remaining, "X-Forwarded-For: %s, %s\r\n", 
                                   x_forwarded_for->value, client_ip);
        if (xff_written > 0 && xff_written < (int)remaining) {
            ptr += xff_written;
            remaining -= xff_written;
        }
    } else {
        // Add new header
        int xff_written = snprintf(ptr, remaining, "X-Forwarded-For: %s\r\n", client_ip);
        if (xff_written > 0 && xff_written < (int)remaining) {
            ptr += xff_written;
            remaining -= xff_written;
        }
    }

    // Add/Update X-Real-IP header
    if (!x_real_ip) {
        int xri_written = snprintf(ptr, remaining, "X-Real-IP: %s\r\n", client_ip);
        if (xri_written > 0 && xri_written < (int)remaining) {
            ptr += xri_written;
            remaining -= xri_written;
        }
    }

    // Copy existing headers (skip Host, X-Forwarded-For, X-Real-IP as we've already handled them)
    const char *headers_start = strstr(original_request, "\r\n");
    if (!headers_start) {
        headers_start = strstr(original_request, "\n");
    }
    if (headers_start) {
        headers_start += 2; // Skip \r\n
        
        // Find end of headers (empty line)
        const char *headers_end = strstr(headers_start, "\r\n\r\n");
        if (!headers_end) {
            headers_end = strstr(headers_start, "\n\n");
            if (headers_end) headers_end += 2;
        } else {
            headers_end += 4;
        }

        if (headers_end) {
            // Parse and copy headers line by line, skipping ones we've replaced
            char *header_copy = strndup(headers_start, headers_end - headers_start);
            if (header_copy) {
                char *line = strtok(header_copy, "\r\n");
                while (line) {
                    // Remove \n if present
                    char *line_end = strchr(line, '\n');
                    if (line_end) *line_end = '\0';
                    
                    // Skip empty lines
                    if (strlen(line) == 0) {
                        line = strtok(NULL, "\r\n");
                        continue;
                    }

                    // Check if this is a header we've already handled
                    char header_name[256];
                    const char *colon = strchr(line, ':');
                    if (colon) {
                        size_t name_len = colon - line;
                        if (name_len < sizeof(header_name)) {
                            strncpy(header_name, line, name_len);
                            header_name[name_len] = '\0';
                            
                            // Skip headers we've already added
                            if (strcasecmp(header_name, "Host") == 0 ||
                                strcasecmp(header_name, "X-Forwarded-For") == 0 ||
                                strcasecmp(header_name, "X-Real-IP") == 0) {
                                line = strtok(NULL, "\r\n");
                                continue;
                            }
                        }
                    }

                    // Copy this header
                    int header_written = snprintf(ptr, remaining, "%s\r\n", line);
                    if (header_written > 0 && header_written < (int)remaining) {
                        ptr += header_written;
                        remaining -= header_written;
                    }

                    line = strtok(NULL, "\r\n");
                }
                free(header_copy);
            }
        } else {
            // No clear header end, copy everything after first line
            const char *body_start = strstr(headers_start, "\r\n\r\n");
            if (!body_start) {
                body_start = strstr(headers_start, "\n\n");
            }
            if (body_start) {
                size_t copy_len = body_start - headers_start;
                if (copy_len < remaining) {
                    // We need to parse and filter, but for simplicity, 
                    // let's just copy and filter manually
                    // This is a fallback - the above parsing should work
                }
            }
        }
    }

    // Add empty line to separate headers from body
    if (remaining >= 2) {
        *ptr++ = '\r';
        *ptr++ = '\n';
        remaining -= 2;
    }

    // Copy body if present
    if (req->body) {
        size_t body_len = strlen(req->body);
        if (body_len < remaining) {
            strncpy(ptr, req->body, body_len);
            ptr += body_len;
        }
    } else {
        // Check if original request had a body (after \r\n\r\n or \n\n)
        const char *body_marker = strstr(original_request, "\r\n\r\n");
        if (body_marker) {
            body_marker += 4; // Skip \r\n\r\n
        } else {
            body_marker = strstr(original_request, "\n\n");
            if (body_marker) {
                body_marker += 2; // Skip \n\n
            }
        }
        if (body_marker && *body_marker != '\0') {
            size_t body_len = strlen(body_marker);
            if (body_len < remaining) {
                strncpy(ptr, body_marker, body_len);
                ptr += body_len;
            }
        }
    }

    *ptr = '\0';
    return modified;
}

