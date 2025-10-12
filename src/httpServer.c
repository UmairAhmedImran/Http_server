#include<sys/socket.h> // for core socket functions
#include<stdlib.h> // does not need as of now will use later for memory
#include<stdio.h>
#include<string.h>
#include<netinet/in.h> // for sockaddr_in struct
#include<unistd.h> // for close function
#include<regex.h>
#include<stdbool.h> 

#define SUCCESS 0
#define FAILURE -1
#define BUFFER_SIZE 4096
#define SERVER_PORT 8000

struct Response {
    int status_code;
    char content_type[64];
    char body[1024];
};


struct Header {
    char key[50];
    char value[256];
};

struct Request {
    char method[16];
    char path[256];
    char version[16];
    struct Header headers[50];
    int header_count;
    char *body;
};

void send_response(int client_socket, struct Response *res) {
    char buffer[2048];

    // Prepare status line
    char *status_text;
    switch (res->status_code) {
        case 200: status_text = "OK"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown";
    }

    // Format full response
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

    // Send the response
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
        // Remove trailing \r if present
        line[strcspn(line, "\r")] = 0;

        if (strlen(line) == 0) {
            // Empty line → headers ended, next lines are body
            in_headers = 0;
            line = strtok_r(NULL, "", &saveptr_outer); // get the rest as body
            if (line && strlen(line) > 0) {
                req->body = strdup(line);
            }
            break;
        }

        if (line_no == 0) {
            // Parse request line: METHOD PATH VERSION
            char *token = strtok_r(line, " ", &saveptr_inner);
            if (token) strncpy(req->method, token, sizeof(req->method));
            token = strtok_r(NULL, " ", &saveptr_inner);
            if (token) strncpy(req->path, token, sizeof(req->path));
            token = strtok_r(NULL, " ", &saveptr_inner);
            if (token) strncpy(req->version, token, sizeof(req->version));
        } 
        else if (in_headers) {
            // Parse headers: key: value
            char *colon_pos = strchr(line, ':');
            if (colon_pos) {
                int index = req->header_count;
                size_t key_len = colon_pos - line;
                strncpy(req->headers[index].key, line, key_len);
                req->headers[index].key[key_len] = '\0';

                // skip ": " space
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



int main(int argc, char *argv[])
{

  // socket variables
  int server_socket, client_socket, c;
  struct sockaddr_in server_addr, client_addr;
  char recv_buffer[BUFFER_SIZE] = {0};
  ssize_t bytes_recv;



  // 1. creating a socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (server_socket == -1)
  {
    perror("Socket creation failed");
    return FAILURE;
  }
  else
  {
    printf("Socket connected successfully\n");
    printf("%d\n", server_socket);
  }

  // 2. assigning values to the server_addr

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = htons(INADDR_ANY); // because we are not concern about the address connection is going to be made on
  
  printf(" server addr port: %d\n", server_addr.sin_port);
  printf("server address family: %d\n", server_addr.sin_family);
  printf("server address: %d\n", server_addr.sin_addr.s_addr);

  // 3. binding to a port
  
  if (bind(server_socket,  (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1  )
  {
    perror("Unable to bind the socket");
    printf("%d", SERVER_PORT );
    close(server_socket);
    return FAILURE;
  } else 
  {
    puts("Binding successfully ");
  }

// 4. Listening to the clients
 
  if( listen(server_socket, 3) == -1) 
  {
    perror("Socket listening failed");
    return FAILURE;
  } else
  {
    printf("Socket listen successfully\n");
  }

  while(1) 
  {
  // 5. accepting the client connections
  puts("waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);
  // LOL was connecting the accept with client_socket thats why it was running continuously and giving error, finally understood how stupid I am
  client_socket = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t *)&c);
  if (client_socket == -1)
  {
    perror("accept failed");
    return FAILURE;
  } 

  bytes_recv = recv(client_socket, recv_buffer, BUFFER_SIZE,  0);

  if (bytes_recv == -1) 
  {
    perror("recv failed");
  } else {
    struct Request req;
    parse_http_request(recv_buffer, &req);

    // Debug print
    printf("Method: %s\n", req.method);
    printf("Path: %s\n", req.path);
    printf("Version: %s\n", req.version);
    printf("\nHeaders:\n");
    for (int i = 0; i < req.header_count; i++) {
        printf("%s: %s\n", req.headers[i].key, req.headers[i].value);
    }
    printf("\nBody:\n%s\n", req.body ? req.body : "(none)");
    struct Response res;
    res.status_code = 200;
    strcpy(res.content_type, "application/json");
    snprintf(res.body, sizeof(res.body),
         "{\"message\": \"Hello from C HTTP Server\", \"path\": \"%s\"}",
         req.path);

    send_response(client_socket, &res);

    free(req.body);
  }

  close(client_socket);
  //close(server_socket);
  }

  return SUCCESS;
}
