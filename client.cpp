#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res;
    int err;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo("127.0.0.1", "3490", &hints, &res);
    if(err != 0) {
	std::cerr << "getaddrinfo() error: " << std::strerror(errno) << std::endl; 
	exit(1);
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd == -1) {
	std::cerr << "socket() error: " << std::strerror(errno) << std::endl; 
	exit(1);
    }

    err = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if(err != 0) {
	std::cerr << "connect() error: " << std::strerror(errno) << std::endl; 
	exit(1);
    }


    return 0;
}
