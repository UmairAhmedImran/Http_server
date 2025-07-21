// Description of database entry of single host 
struct hostnet 
{
  char *h_name;         // Official name of host
  char **h_aliases;     // Alias List
  int h_addrtype;       // Host address type
  int h_length;         // Length of address
  char **h_addr_list;   // list of address from name server
};
