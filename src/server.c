#include "../include/server.h"
#include "../include/http.h"
#include "../include/backend.h"
#include "../include/logging.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

pthread_mutex_t backend_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  int client_socket;
  char recv_buffer[BUFFER_SIZE];
  struct sockaddr_in client_addr;
} thread_data_t;

void handle_client(int client_socket, char *recv_buffer,
                   struct sockaddr_in client_addr) {
  struct Request req;
  req.body = NULL;

  char parse_buffer[BUFFER_SIZE];
  if (strlen(recv_buffer) >= BUFFER_SIZE) {
    log_error("server", "buffer_overflow", "Request buffer too large");
    close(client_socket);
    return;
  }
  strncpy(parse_buffer, recv_buffer, BUFFER_SIZE - 1);
  parse_buffer[BUFFER_SIZE - 1] = '\0';

  parse_http_request(parse_buffer, &req);

  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

  log_message(LOG_INFO, "Received request from %s:%d", client_ip,
              ntohs(client_addr.sin_port));
  log_message(LOG_DEBUG, "Request: %s %s %s", req.method, req.path,
              req.version);

  pthread_mutex_lock(&backend_mutex);

  int any_active = 0;
  for (int i = 0; i < backend_pool.count; i++) {
    if (backend_pool.backends[i].is_active) {
      any_active = 1;
      break;
    }
  }

  pthread_mutex_unlock(&backend_mutex);

  if (!any_active) {
    log_error("server", "no_backend_available", "No backend servers available");

    struct Response res;
    res.status_code = 503;
    strncpy(res.content_type, "application/json", sizeof(res.content_type) - 1);
    res.content_type[sizeof(res.content_type) - 1] = '\0';
    snprintf(res.body, sizeof(res.body),
             "{\"error\": \"Service Unavailable\", \"message\": \"No backend "
             "servers available\"}");

    send_response(client_socket, &res);
    log_request(req.method, req.path, "none", 503);

    if (req.body) {
      free(req.body);
      req.body = NULL;
    }
    close(client_socket);
    return;
  }

  int forward_result = -1;
  struct Backend *used_backend = NULL;
  char *backend_response = NULL;
  ssize_t backend_response_size = 0;
  int max_attempts = backend_pool.count;
  int attempts = 0;

  while (attempts < max_attempts) {
    pthread_mutex_lock(&backend_mutex);
    struct Backend *candidate = get_next_backend(&backend_pool);
    
    if (!candidate) {
      pthread_mutex_unlock(&backend_mutex);
      log_error("server", "no_backend_available", "No backends available for load balancing");
      break;
    }

    log_message(LOG_INFO, "Selected backend %s:%d for request %s %s (weight: %d)",
                candidate->host, candidate->port, req.method, req.path, candidate->weight);

    struct Backend backend_copy;
    strncpy(backend_copy.host, candidate->host, sizeof(backend_copy.host) - 1);
    backend_copy.host[sizeof(backend_copy.host) - 1] = '\0';
    backend_copy.port = candidate->port;

    pthread_mutex_unlock(&backend_mutex);

    char *modified_request = modify_request_for_proxy(recv_buffer, &req, client_ip, &backend_copy);
    if (!modified_request) {
        log_error("server", "request_modification_failed", 
                  "Failed to modify request for proxy compatibility");
        attempts++;
        continue;
    }

    forward_result = forward_to_backend(candidate, modified_request, 
                                       &backend_response, &backend_response_size);
    free(modified_request);
    
    if (forward_result == 0) {
      used_backend = candidate;
      pthread_mutex_lock(&backend_mutex);
      if (!candidate->is_active) {
        candidate->is_active = true;
        for (int i = 0; i < backend_pool.count; i++) {
          if (&backend_pool.backends[i] == candidate) {
            backend_pool.current_weights[i] = candidate->weight;
            break;
          }
        }
        log_message(LOG_INFO, "Backend %s:%d recovered and is now active (request succeeded)", 
                   candidate->host, candidate->port);
      }
      pthread_mutex_unlock(&backend_mutex);
      break;
    }

    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), 
             "Failed to forward request to backend %s:%d, trying next if available",
             candidate->host, candidate->port);
    log_error("server", "forward_failed", error_msg);
    attempts++;
  }

  char backend_with_port[100];

  if (used_backend && forward_result == 0 && backend_response && backend_response_size > 0) {
    snprintf(backend_with_port, sizeof(backend_with_port), "%s:%d",
             used_backend->host, used_backend->port);

    ssize_t sent = send(client_socket, backend_response, backend_response_size, 0);
    
    if (sent < 0) {
      char err_msg[256];
      snprintf(err_msg, sizeof(err_msg), "Failed to send backend response to client: %s (errno: %d)",
               strerror(errno), errno);
      log_error("server", "send_to_client_failed", err_msg);
    } else if (sent != backend_response_size) {
      log_message(LOG_WARNING, "Partial send to client (%zd/%zd bytes)", 
                 sent, backend_response_size);
    } else {
      log_message(LOG_DEBUG, "Successfully forwarded %zd bytes from backend to client", sent);
    }

    int status_code = 200;
    if (backend_response_size > 12) {
      char status_str[4] = {0};
      const char *status_pos = strstr(backend_response, "HTTP/");
      if (status_pos) {
        const char *code_start = strchr(status_pos, ' ');
        if (code_start) {
          code_start++;
          strncpy(status_str, code_start, 3);
          status_str[3] = '\0';
          status_code = atoi(status_str);
        }
      }
    }

    log_request(req.method, req.path, backend_with_port, status_code);
    free(backend_response);
    backend_response = NULL;
  } else {
    struct Response res;
    res.status_code = 502;
    strncpy(res.content_type, "application/json", sizeof(res.content_type) - 1);
    res.content_type[sizeof(res.content_type) - 1] = '\0';
    snprintf(res.body, sizeof(res.body),
             "{\"error\": \"Bad Gateway\", \"message\": \"Failed to forward to "
             "any backend server\"}");

    if (any_active) {
      snprintf(backend_with_port, sizeof(backend_with_port), "multiple/failed");
    } else {
      snprintf(backend_with_port, sizeof(backend_with_port), "none");
    }

    send_response(client_socket, &res);
    log_request(req.method, req.path, backend_with_port, 502);
    
    if (backend_response) {
      free(backend_response);
      backend_response = NULL;
    }
  }

  if (req.body) {
    free(req.body);
    req.body = NULL;
  }
  
  if (close(client_socket) < 0) {
    char err_msg[256];
    snprintf(err_msg, sizeof(err_msg), "Failed to close client socket: %s (errno: %d)",
             strerror(errno), errno);
    log_error("server", "close_failed", err_msg);
  }
  log_message(LOG_DEBUG, "Client connection closed");
}

void *client_thread_handler(void *arg) {
  thread_data_t *data = (thread_data_t *)arg;

  handle_client(data->client_socket, data->recv_buffer, data->client_addr);

  free(data);

  return NULL;
}

int start_server(int port) {
  int server_socket, client_socket;
  struct sockaddr_in server_addr, client_addr;
  char recv_buffer[BUFFER_SIZE];
  socklen_t c = sizeof(struct sockaddr_in);
  pthread_t thread_id;

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    char err_msg[256];
    snprintf(err_msg, sizeof(err_msg), "Failed to create server socket: %s (errno: %d)",
             strerror(errno), errno);
    log_error("server", "socket_creation_failed", err_msg);
    return FAILURE;
  }

  int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    char err_msg[256];
    snprintf(err_msg, sizeof(err_msg), "Failed to set socket options: %s (errno: %d)",
             strerror(errno), errno);
    log_error("server", "setsockopt_failed", err_msg);
    close(server_socket);
    return FAILURE;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) == -1) {
    char err_msg[256];
    snprintf(err_msg, sizeof(err_msg), "Failed to bind socket to port %d: %s (errno: %d)",
             port, strerror(errno), errno);
    log_error("server", "bind_failed", err_msg);
    close(server_socket);
    return FAILURE;
  }

  if (listen(server_socket, 5) == -1) {
    char err_msg[256];
    snprintf(err_msg, sizeof(err_msg), "Failed to listen on socket: %s (errno: %d)",
             strerror(errno), errno);
    log_error("server", "listen_failed", err_msg);
    close(server_socket);
    return FAILURE;
  }

  log_message(LOG_INFO, "Server listening on port %d", port);
  log_message(LOG_INFO, "Server ready to accept connections");

  while (1) {
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &c);
    if (client_socket == -1) {
      char err_msg[256];
      snprintf(err_msg, sizeof(err_msg), "Failed to accept client connection: %s (errno: %d)",
               strerror(errno), errno);
      log_error("server", "accept_failed", err_msg);
      continue;
    }

    ssize_t bytes_recv = recv(client_socket, recv_buffer, BUFFER_SIZE - 1, 0);
    if (bytes_recv <= 0) {
      if (bytes_recv == 0) {
        log_message(LOG_WARNING, "Client disconnected before sending data");
      } else {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Error receiving data from client: %s (errno: %d)",
                 strerror(errno), errno);
        log_error("server", "receive_failed", err_msg);
      }
      close(client_socket);
      continue;
    }
    
    if (bytes_recv >= BUFFER_SIZE - 1) {
      log_error("server", "buffer_overflow", "Received data exceeds buffer size");
      close(client_socket);
      continue;
    }

    recv_buffer[bytes_recv] = '\0';

    thread_data_t *data = malloc(sizeof(thread_data_t));
    if (data == NULL) {
      log_error("server", "malloc",
                "failed to allocate memory for thread data");
      close(client_socket);
      continue;
    }

    data->client_socket = client_socket;
    memcpy(data->recv_buffer, recv_buffer, BUFFER_SIZE);
    data->client_addr = client_addr;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&thread_id, &attr, client_thread_handler, data) != 0) {
      log_error("server", "thread_creation_failed",
                "Failed to create thread for client handling");
      free(data);
      close(client_socket);
      pthread_attr_destroy(&attr);
      continue;
    }
    pthread_attr_destroy(&attr);
  }

  close(server_socket);
  pthread_mutex_destroy(&backend_mutex);
  return SUCCESS;
}
