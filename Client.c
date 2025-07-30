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
