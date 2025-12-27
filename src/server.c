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

pthread_mutex_t backend_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
  int client_socket;
  char recv_buffer[BUFFER_SIZE];
  struct sockaddr_in client_addr;
} thread_data_t;

void handle_client(int client_socket, char *recv_buffer,
                   struct sockaddr_in client_addr) {
  struct Request req;

  // Parse the HTTP request using a COPY of the buffer so we don't mutate
  // the raw bytes that will be forwarded to the backend.
  char parse_buffer[BUFFER_SIZE];
  strncpy(parse_buffer, recv_buffer, BUFFER_SIZE - 1);
  parse_buffer[BUFFER_SIZE - 1] = '\0';

  parse_http_request(parse_buffer, &req);

  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

  log_message(LOG_INFO, "Received request from %s:%d", client_ip,
              ntohs(client_addr.sin_port));
  log_message(LOG_DEBUG, "Request: %s %s %s", req.method, req.path,
              req.version);

  // Failover strategy:
  //  - Try backends in fixed order: 9001, then 9002, then 9003
  //  - Skip any backend already marked inactive
  //  - On failure, try the next backend for THIS request
  //  - If all attempts fail, return 502 to the client

  // locking mutex for before accessing backend_pool
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
    strcpy(res.content_type, "application/json");
    snprintf(res.body, sizeof(res.body),
             "{\"error\": \"Service Unavailable\", \"message\": \"No backend "
             "servers available\"}");

    send_response(client_socket, &res);
    log_request(req.method, req.path, "none", 503);

    free(req.body);
    close(client_socket);
    return;
  }

  int forward_result = -1;
  struct Backend *used_backend = NULL;
  char *backend_response = NULL;
  ssize_t backend_response_size = 0;

  pthread_mutex_lock(&backend_mutex);

  // Try each backend, including inactive ones (they might have recovered)
  for (int i = 0; i < backend_pool.count; i++) {
    struct Backend *candidate = &backend_pool.backends[i];

    // Skip inactive backends only if we have active ones to try first
    // But if all are inactive, try them anyway (they might have recovered)
    int has_active = 0;
    for (int j = 0; j < backend_pool.count; j++) {
      if (backend_pool.backends[j].is_active) {
        has_active = 1;
        break;
      }
    }
    
    if (!candidate->is_active && has_active) {
      continue; // Skip inactive backends if we have active ones
    }

    log_message(LOG_INFO, "Trying backend %s:%d for request %s %s (active: %s)",
                candidate->host, candidate->port, req.method, req.path,
                candidate->is_active ? "true" : "false");

    // Copy backend info before unlocking mutex (we need it for modify_request_for_proxy)
    struct Backend backend_copy;
    strncpy(backend_copy.host, candidate->host, sizeof(backend_copy.host) - 1);
    backend_copy.host[sizeof(backend_copy.host) - 1] = '\0';
    backend_copy.port = candidate->port;

    pthread_mutex_unlock(&backend_mutex);

    // Modify request to add proxy headers (X-Forwarded-For, Host, etc.)
    // This doesn't need mutex protection as it only modifies the request string
    char *modified_request = modify_request_for_proxy(recv_buffer, &req, client_ip, &backend_copy);
    if (!modified_request) {
        log_error("server", "request_modification_failed", 
                  "Failed to modify request for proxy compatibility");
        pthread_mutex_lock(&backend_mutex);
        continue;
    }

    forward_result = forward_to_backend(candidate, modified_request, 
                                       &backend_response, &backend_response_size);
    
    // Free the modified request after forwarding
    free(modified_request);
    
    pthread_mutex_lock(&backend_mutex);
    if (forward_result == 0) {
      used_backend = candidate;
      // If backend was inactive but now works, mark it active again
      if (!candidate->is_active) {
        candidate->is_active = true;
        log_message(LOG_INFO, "Backend %s:%d recovered and is now active", 
                   candidate->host, candidate->port);
      }
      break;
    }

    // forward_to_backend already logs and may mark backend inactive.
    log_error("server", "forward_failed",
              "Failed to forward request to backend, trying next if available");
  }

  pthread_mutex_unlock(&backend_mutex);

  char backend_with_port[100];

  if (used_backend && forward_result == 0 && backend_response && backend_response_size > 0) {
    snprintf(backend_with_port, sizeof(backend_with_port), "%s:%d",
             used_backend->host, used_backend->port);

    // Forward the backend's HTTP response directly to the client
    ssize_t sent = send(client_socket, backend_response, backend_response_size, 0);
    
    if (sent < 0) {
      log_error("server", "send_to_client_failed", "Failed to send backend response to client");
    } else if (sent != backend_response_size) {
      log_message(LOG_WARNING, "Partial send to client (%zd/%zd bytes)", 
                 sent, backend_response_size);
    } else {
      log_message(LOG_DEBUG, "Successfully forwarded %zd bytes from backend to client", sent);
    }

    // Extract status code from response for logging (simple parsing)
    int status_code = 200; // default
    if (backend_response_size > 12) {
      // HTTP response starts with "HTTP/1.x STATUS_CODE"
      // Try to parse status code from response
      char status_str[4] = {0};
      const char *status_pos = strstr(backend_response, "HTTP/");
      if (status_pos) {
        // Find space after HTTP version, then read status code
        const char *code_start = strchr(status_pos, ' ');
        if (code_start) {
          code_start++; // skip space
          strncpy(status_str, code_start, 3);
          status_code = atoi(status_str);
        }
      }
    }

    log_request(req.method, req.path, backend_with_port, status_code);
    
    // Free the backend response buffer
    free(backend_response);
    backend_response = NULL;
  } else {
    // All backends failed for this request
    struct Response res;
    res.status_code = 502;
    strcpy(res.content_type, "application/json");
    snprintf(res.body, sizeof(res.body),
             "{\"error\": \"Bad Gateway\", \"message\": \"Failed to forward to "
             "any backend server\"}");

    // Best-effort backend string (may be "none" if nothing active)
    if (any_active) {
      snprintf(backend_with_port, sizeof(backend_with_port), "multiple/failed");
    } else {
      snprintf(backend_with_port, sizeof(backend_with_port), "none");
    }

    send_response(client_socket, &res);
    log_request(req.method, req.path, backend_with_port, 502);
    
    // Clean up if we have a partial response
    if (backend_response) {
      free(backend_response);
      backend_response = NULL;
    }
  }

  free(req.body);
  close(client_socket);
  log_message(LOG_DEBUG, "Client connection closed");
}

void *client_thread_handler(void *arg) {
  thread_data_t *data = (thread_data_t *)arg;

  handle_client(data->client_socket, data->recv_buffer, data->client_addr);

  free(data);

  return NULL;
}

int start_server() {
  int server_socket, client_socket;
  struct sockaddr_in server_addr, client_addr;
  char recv_buffer[BUFFER_SIZE];
  socklen_t c = sizeof(struct sockaddr_in);
  pthread_t thread_id;

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    log_error("server", "socket_creation_failed",
              "Failed to create server socket");
    return FAILURE;
  }

  int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    log_error("server", "setsockopt_failed", "Failed to set socket options");
    close(server_socket);
    return FAILURE;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) == -1) {
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
      log_error("server", "accept_failed",
                "Failed to accept client connection");
      continue;
    }

    ssize_t bytes_recv = recv(client_socket, recv_buffer, BUFFER_SIZE - 1, 0);
    if (bytes_recv <= 0) {
      if (bytes_recv == 0) {
        log_message(LOG_WARNING, "Client disconnected before sending data");
      } else {
        log_error("server", "receive_failed",
                  "Error receiving data from client");
      }
      close(client_socket);
      continue;
    }

    recv_buffer[bytes_recv] = '\0';

    // Allocate memory for thread data
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
