#include <cerrno>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

const char *PORT = "3490";
const int MAXDATASIZE = 100;

void *get_in_addr(sockaddr *sa)
{
    if(sa->sa_family == AF_INET) { 
	return &(((sockaddr_in*)sa)->sin_addr);
    }
    return &(((sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    if(argc != 2) {
	std::cerr << "Usage: client <hostname>" << std::endl;
	return 1;
    }

    addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *servinfo;
    if(0 != getaddrinfo(argv[1], PORT, &hints, &servinfo)) {
	std::cerr << "getaddrinfo() error: " << std::strerror(errno) << std::endl; 
	return 1;
    }
    int sockfd; 
    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(-1 == (sockfd = socket(servinfo->ai_family,
			      servinfo->ai_socktype,
			      servinfo->ai_protocol))) {
	std::cerr << "socket() error: " << std::strerror(errno) << std::endl; 
	return 1;
    }

    addrinfo *p;
    char addrstr[INET6_ADDRSTRLEN];
    for(p = servinfo; p != NULL; p=p->ai_next) { 
	if(-1 == (sockfd = socket(p->ai_family,
			          p->ai_socktype,
				  p->ai_protocol))) {
    	    std::cerr << "socket() error: " << std::strerror(errno) << std::endl; 
	    continue;
    	}
	inet_ntop(p->ai_family,
		  get_in_addr((sockaddr*)&p->ai_addr),
		  addrstr, sizeof addrstr);
	std::cout << "Client: attempting connection to " << addrstr << std::endl;

	if(-1 == connect(sockfd, p->ai_addr, p->ai_addrlen)) {
	    std::cerr << "connect() error: " << std::strerror(errno) << std::endl;
	    close(sockfd);
	    continue;
    	}
	break;
    }

    if(p == NULL) {
	std::cerr << "Failed to connect" << std::endl;
	return 2;
    }
    inet_ntop(p->ai_family,
	      get_in_addr((sockaddr*)&p->ai_addr),
	      addrstr, sizeof addrstr);
    std::cout << "Client: connected to " << addrstr << std::endl;

    freeaddrinfo(servinfo);

    int numbytes;
    char buf[MAXDATASIZE];
    if(-1 == (numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0))) {
	std::cerr << "recv() error: " << std::strerror(errno) << std::endl; 
	return 3;
    }

    std::cout << "Client: recieved " << buf << std::endl;

    close(sockfd);

    return 0;
}
