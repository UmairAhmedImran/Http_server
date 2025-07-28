#include<stdio.h>
#include<sys/socketvar.h>
#include<stdlib.h>
#include<arpa/inet.h>

// IPV4 AF_INET sockets
//struct in_addr {
//  unsigned long s_addr;             // load with inet_pton 
//};
//
//struct sockaddr_in {
//  short           sin_family;       //e.g AF_INET, AF_INET6
//  unsigned short  sin_port;         //e.g, htons(3490)
//  struct in_addr  sin_addr;         // see struct in_addr, below
//  char            sin_zero[8];      // zero this if you want to
//};
//
//struct sock_addr {
//  unsigned short    sa_family;      // address family, AF_xxxx
//  char              sa_data[14];     // 14 bytes of protocol address
//};


int main(int argc ,char *argv[])
{
  int socket_desc, new_socket, c;
  struct sockaddr_in server, client;

  socket_desc = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_desc == EXIT_FAILURE) 
  {
    printf("Could not create socket");
    printf("%d", EXIT_FAILURE);
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(8888);

  if(bind(socket_desc, (struct sockaddr *)&server , sizeof(server)) < 0 )
  {
    puts("bind failed");
  }
 
  puts("bind done");

  listen(socket_desc, 3);

  puts("waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);
  new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
  
  if (new_socket < 0) 
  {
    perror("accept failed");
  }

  puts("Connection Accepted");

  return 0; 
}



  //struct sockaddr_in server;
//server.sin_port = htons(80);
//server.sin_addr.sin_family = inet_addr("")

