#ifndef SERVER_H
#define SERVER_H

#include "networkfuncs.h"

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

namespace libhttp 
{
class Server
{
    int sockfd;
    std::string ip;
    std::string hostname;
    int port;
    // Returns socket fd
    int create_socket(const std::string &host, int port,
			   		  /*int address_family,*/ int socket_flags = 0);    
	bool fill_socket_info(const std::string &host);
	bool handle_connection(int client_socket);

public:
    bool listen(const std::string &host, int port);
};

}

#endif
