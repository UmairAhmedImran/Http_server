#ifndef SERVER_H
#define SERVER_H

#include "backend.h" 

#define BUFFER_SIZE 4096
#define SERVER_PORT 8080
#define SUCCESS 0
#define FAILURE -1

extern struct BackendPool backend_pool;

int start_server(void);

#endif