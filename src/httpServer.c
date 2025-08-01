#include<sys/socket.h> // for core socket functions
#include<stdlib.h>
#include<stdio.h>
#include<netinet/in.h> // for sockaddr_in struct
#include<unistd.h> // for close function

int main(int argc, char *argv[])
{
  int server_socket;
  struct sockaddr_in server_addr;
  // 1. creating a socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (server_socket == -1)
  {
    perror("Socket creation failed");
    return EXIT_FAILURE;
  }
  else
  {
    printf("Socket connected successfully\n");
    printf("%d\n", server_socket);
  }

  // 2. assigning values to the server_addr

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8080);
  server_addr.sin_addr.s_addr = INADDR_ANY; // because we are not concern about the address connection is going to be made on
  
  // 3. binding to a port
  
  if (bind(server_socket,  (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1  )
  {
    perror("Unable to bind the socket");
    close(server_socket);
    return EXIT_FAILURE;
  }

  listen(server_socket, 3);

  return EXIT_SUCCESS;
}
