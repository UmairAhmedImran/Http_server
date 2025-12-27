#include "../include/http.h"
#include "../include/backend.h"
#include "../include/logging.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <strings.h>  // for strcasecmp
#include <errno.h>

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
            if (token) {
                strncpy(req->method, token, sizeof(req->method) - 1);
                req->method[sizeof(req->method) - 1] = '\0';
            }
            token = strtok_r(NULL, " ", &saveptr_inner);
            if (token) {
                strncpy(req->path, token, sizeof(req->path) - 1);
                req->path[sizeof(req->path) - 1] = '\0';
            }
            token = strtok_r(NULL, " ", &saveptr_inner);
            if (token) {
                strncpy(req->version, token, sizeof(req->version) - 1);
                req->version[sizeof(req->version) - 1] = '\0';
            }
        } else if (in_headers) {
            char *colon_pos = strchr(line, ':');
            if (colon_pos && req->header_count < MAX_HEADERS) {
                int index = req->header_count;
                size_t key_len = colon_pos - line;
                size_t max_key_len = sizeof(req->headers[index].key) - 1;
                if (key_len > max_key_len) {
                    key_len = max_key_len;
                }
                strncpy(req->headers[index].key, line, key_len);
                req->headers[index].key[key_len] = '\0';
                char *value = colon_pos + 1;
                while (*value == ' ') value++;
                size_t max_value_len = sizeof(req->headers[index].value) - 1;
                size_t value_len = strlen(value);
                if (value_len > max_value_len) {
                    value_len = max_value_len;
                }
                strncpy(req->headers[index].value, value, value_len);
                req->headers[index].value[value_len] = '\0';
                req->header_count++;
            }
        }

        line_no++;
        line = strtok_r(NULL, "\n", &saveptr_outer);
    }
}

static struct Header* find_header(struct Request *req, const char *name) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, name) == 0) {
            return &req->headers[i];
        }
    }
    return NULL;
}

char* modify_request_for_proxy(const char *original_request, 
                                struct Request *req,
                                const char *client_ip,
                                struct Backend *backend) {
    if (!original_request || !req || !client_ip || !backend) {
        return NULL;
    }

    size_t original_len = strlen(original_request);
    size_t buffer_size = original_len + 512;
    char *modified = malloc(buffer_size);
    if (!modified) {
        return NULL;
    }

    struct Header *x_forwarded_for = find_header(req, "X-Forwarded-For");
    struct Header *x_real_ip = find_header(req, "X-Real-IP");

    char *ptr = modified;
    size_t remaining = buffer_size;

    const char *request_line_end = strstr(original_request, "\r\n");
    if (!request_line_end) {
        request_line_end = strstr(original_request, "\n");
    }
    if (request_line_end) {
        size_t line_len = request_line_end - original_request;
        if (line_len < remaining) {
            if (line_len < remaining) {
                strncpy(ptr, original_request, line_len);
                ptr[line_len] = '\0';
                ptr += line_len;
                remaining -= line_len;
                if (remaining >= 2) {
                    *ptr++ = '\r';
                    *ptr++ = '\n';
                    remaining -= 2;
                }
            } else {
                log_error("http", "buffer_overflow", "Request line too long");
                free(modified);
                return NULL;
            }
        }
    } else {
        const char *first_newline = strchr(original_request, '\n');
        if (first_newline) {
            size_t line_len = first_newline - original_request;
            if (line_len < remaining) {
                strncpy(ptr, original_request, line_len);
                ptr[line_len] = '\0';
                ptr += line_len;
                remaining -= line_len;
                if (remaining >= 2) {
                    *ptr++ = '\r';
                    *ptr++ = '\n';
                    remaining -= 2;
                }
            } else {
                log_error("http", "buffer_overflow", "Request line too long");
                free(modified);
                return NULL;
            }
        }
    }

    char host_value[100];
    int host_snprintf_result = snprintf(host_value, sizeof(host_value), "%s:%d", backend->host, backend->port);
    if (host_snprintf_result < 0 || host_snprintf_result >= (int)sizeof(host_value)) {
        log_error("http", "host_value_overflow", "Host value too long");
        free(modified);
        return NULL;
    }
    int host_written = snprintf(ptr, remaining, "Host: %s\r\n", host_value);
    if (host_written > 0 && host_written < (int)remaining) {
        ptr += host_written;
        remaining -= host_written;
    } else if (host_written >= (int)remaining) {
        log_error("http", "buffer_overflow", "Not enough space for Host header");
        free(modified);
        return NULL;
    }

    if (x_forwarded_for) {
        int xff_written = snprintf(ptr, remaining, "X-Forwarded-For: %s, %s\r\n", 
                                   x_forwarded_for->value, client_ip);
        if (xff_written > 0 && xff_written < (int)remaining) {
            ptr += xff_written;
            remaining -= xff_written;
        }
    } else {
        int xff_written = snprintf(ptr, remaining, "X-Forwarded-For: %s\r\n", client_ip);
        if (xff_written > 0 && xff_written < (int)remaining) {
            ptr += xff_written;
            remaining -= xff_written;
        }
    }

    if (!x_real_ip) {
        int xri_written = snprintf(ptr, remaining, "X-Real-IP: %s\r\n", client_ip);
        if (xri_written > 0 && xri_written < (int)remaining) {
            ptr += xri_written;
            remaining -= xri_written;
        }
    }

    const char *headers_start = strstr(original_request, "\r\n");
    if (!headers_start) {
        headers_start = strstr(original_request, "\n");
    }
    if (headers_start) {
        headers_start += 2;
        
        const char *headers_end = strstr(headers_start, "\r\n\r\n");
        if (!headers_end) {
            headers_end = strstr(headers_start, "\n\n");
            if (headers_end) headers_end += 2;
        } else {
            headers_end += 4;
        }

        if (headers_end) {
            char *header_copy = strndup(headers_start, headers_end - headers_start);
            if (header_copy) {
                char *line = strtok(header_copy, "\r\n");
                while (line) {
                    char *line_end = strchr(line, '\n');
                    if (line_end) *line_end = '\0';
                    
                    if (strlen(line) == 0) {
                        line = strtok(NULL, "\r\n");
                        continue;
                    }

                    char header_name[256];
                    const char *colon = strchr(line, ':');
                    if (colon) {
                        size_t name_len = colon - line;
                        if (name_len < sizeof(header_name)) {
                            size_t copy_len = (name_len < sizeof(header_name) - 1) ? name_len : sizeof(header_name) - 1;
                            strncpy(header_name, line, copy_len);
                            header_name[copy_len] = '\0';
                            
                            if (strcasecmp(header_name, "Host") == 0 ||
                                strcasecmp(header_name, "X-Forwarded-For") == 0 ||
                                strcasecmp(header_name, "X-Real-IP") == 0) {
                                line = strtok(NULL, "\r\n");
                                continue;
                            }
                        }
                    }

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
            const char *body_start = strstr(headers_start, "\r\n\r\n");
            if (!body_start) {
                body_start = strstr(headers_start, "\n\n");
            }
            if (body_start) {
                size_t copy_len = body_start - headers_start;
                if (copy_len < remaining) {
                }
            }
        }
    }

    if (remaining >= 2) {
        *ptr++ = '\r';
        *ptr++ = '\n';
        remaining -= 2;
    }

    if (req->body) {
        size_t body_len = strlen(req->body);
        if (body_len < remaining) {
            strncpy(ptr, req->body, body_len);
            ptr[body_len] = '\0';
            ptr += body_len;
        } else {
            log_message(LOG_WARNING, "Request body truncated (size: %zu, remaining: %zu)", 
                       body_len, remaining);
            strncpy(ptr, req->body, remaining - 1);
            ptr[remaining - 1] = '\0';
            ptr += (remaining - 1);
        }
    } else {
        const char *body_marker = strstr(original_request, "\r\n\r\n");
        if (body_marker) {
            body_marker += 4;
        } else {
            body_marker = strstr(original_request, "\n\n");
            if (body_marker) {
                body_marker += 2;
            }
        }
        if (body_marker && *body_marker != '\0') {
            size_t body_len = strlen(body_marker);
            if (body_len < remaining) {
                strncpy(ptr, body_marker, body_len);
                ptr[body_len] = '\0';
                ptr += body_len;
            } else {
                strncpy(ptr, body_marker, remaining - 1);
                ptr[remaining - 1] = '\0';
                ptr += (remaining - 1);
            }
        }
    }

    *ptr = '\0';
    return modified;
}

