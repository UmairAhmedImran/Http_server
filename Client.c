// Description of database entry of single host 
//struct hostnet 
//{
//  char *h_name;         // Official name of host
//  char **h_aliases;     // Alias List
//  int h_addrtype;       // Host address type
//  int h_length;         // Length of address
//  char **h_addr_list;   // list of address from name server
//};

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include<stdlib.h>
#include <unistd.h> /* close */


int main(int argc, char *argv[])
{
   struct hostent *h;
  
  h = gethostbyname(argv[1]);
  if(h==NULL) 
  {
    printf("%s: unknown host '%s'\n",argv[0],argv[1]);
    EXIT_FAILURE;
  }

}
