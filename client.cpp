#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

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

    sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);
    getpeername(sockfd, (sockaddr*)&peer_addr, &peer_addr_len);

    char hoststr[NI_MAXHOST];
    char servstr[NI_MAXSERV];
    char hostnamestr[256];
    
    gethostname(hostnamestr, 256);
    std::cout << "Your computer name is: " << hostnamestr << std::endl;

    err = getnameinfo((sockaddr *)&peer_addr,
    		      peer_addr_len,
    		      hoststr,
    		      sizeof(hoststr),
    		      servstr,
    		      sizeof(servstr),
    		      NI_NUMERICHOST | NI_NUMERICSERV);

    std::cout << "The peer addr is: " << hoststr << ':' << servstr << std::endl;

    char buf[256];
    recv(sockfd, buf, sizeof(char)*256, 0);

    shutdown(sockfd, SHUT_RD);

    std::cout << "From server: " << buf << std::endl;

    char send_buf[256];
    snprintf(send_buf, 256, "Message received");
    send(sockfd, send_buf, 256, 0); 

    shutdown(sockfd, SHUT_WR);

    close(sockfd);

    return 0;
}
