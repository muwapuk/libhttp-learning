#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>

int main(int argc, char *argv[])
{
    const int BACKLOG = 10;
    const char* MYPORT = "3490";

    addrinfo hints, *res;
    sockaddr_storage their_addr;
    socklen_t their_addr_size;
    int err;
    int listen_sock;
    int client_sock;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(NULL, MYPORT, &hints, &res);
    if(err != 0) {
	std::cerr << "getaddrinfo() error: " << std::strerror(errno) << std::endl; 
	exit(1);
    }
    
    listen_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(listen_sock == -1) {
	std::cerr << "socket() error: " << std::strerror(errno) << std::endl; 
	exit(1);
    }
 

    int yes=1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    err = bind(listen_sock, res->ai_addr, res->ai_addrlen); 
    if(err != 0) {
	std::cerr << "bind() error: " << std::strerror(errno) << std::endl; 
	exit(1);
    }
   
    err = listen(listen_sock, BACKLOG);
    if(err != 0) {
	std::cerr << "listen() error: " << std::strerror(errno) << std::endl; 
	exit(1);
    }

    their_addr_size = sizeof(their_addr);
    client_sock = accept(listen_sock, (sockaddr*)&their_addr, &their_addr_size);
////
    char hoststr[NI_MAXHOST];
    char servstr[NI_MAXSERV];
    err = getnameinfo((sockaddr *)&their_addr,
		      their_addr_size,
		      hoststr,
		      sizeof(hoststr),
		      servstr,
		      sizeof(servstr),
		      NI_NUMERICHOST | NI_NUMERICSERV);
/////
    std::cout << "Connection accepted: "
	      << "Socket descriptor " << client_sock << " - "
	      << hoststr << ':' << servstr
	      << std::endl; 

    return 0;
}
