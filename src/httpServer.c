#include<sys/socket.h> // for core socket functions
//#include<stdlib.h> // does not need as of now will use later for memory
#include<stdio.h>
#include<string.h>
#include<netinet/in.h> // for sockaddr_in struct
#include<unistd.h> // for close function
#include<regex.h>

#define SUCCESS 0
#define FAILURE -1
#define BUFFER_SIZE 4096
#define SERVER_PORT 8080

int main(int argc, char *argv[])
{
  int server_socket, client_socket, c;
  struct sockaddr_in server_addr, client_addr;
  char *buffer = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Lenght: 18\r\n\r\n"
    "{message: ok}\n";
  char recv_buffer[BUFFER_SIZE] = {0};
  ssize_t bytes_send, bytes_recv;
  
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

  bytes_send = send(client_socket, buffer, strlen(buffer), 0);

  if (bytes_send == -1)
  {
    perror("send failed");
  } else {
    printf("Sent %zd bytes to client.\n", bytes_send);
  }

  bytes_recv = recv(client_socket, recv_buffer, BUFFER_SIZE,  0);

  if (bytes_recv == -1) 
  {
    perror("recv failed");
  } else {
    printf("%s\n", recv_buffer);
    char *ptr_client_data_by_line = strtok(recv_buffer, "\n"); // usign strtok as of now but should use strtok_r for multithreading later
    while (ptr_client_data_by_line != NULL)
    {
    printf("formattting the data --------  %s\n", ptr_client_data_by_line);
    ptr_client_data_by_line = strtok(NULL, "\n");
    }
  }
  
  

  close(client_socket);
  //close(server_socket);
  }

  return SUCCESS;
}
