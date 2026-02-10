#ifndef CLIENT_H
#define CLIENT_H
#include <cerrno>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

namespace libhttp
{
void *get_in_addr(sockaddr *sa)
{
    if(sa->sa_family == AF_INET) { 
	return &(((sockaddr_in*)sa)->sin_addr);
    }
    return &(((sockaddr_in6*)sa)->sin6_addr);
}
class Client
{
    Client(int maxdata): MAXDATASIZE(maxdata) {};
    /*
    * removed many unused <sockfd> reassignments
    * fixed tabulation
    * also it looks like <Address> structure is obsolete
    * 
    * only constant we have is <MAXDATASIZE>
    * address and port we recieve when we want to connect somewhere
    * and can be changed any time we want to connect_to(some, where)
    *
    * however maybe it's important to check if connection already exist
    * before creating a new one
    */

    const int MAXDATASIZE;
    addrinfo hints, *servinfo;
    int sockfd;
public:
    int connect_to(std::string addr, std::string port)
    {
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if(0 != getaddrinfo(addr.c_str(), port.c_str(), &hints, &servinfo)) {
            std::cerr << "getaddrinfo() error: " << std::strerror(errno) << std::endl; 
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
                    addrstr,
                    sizeof addrstr);
            std::cout << "Client: attempting connection to " << addrstr << std::endl;

            if(-1 == connect(sockfd,
                        p->ai_addr,
                        p->ai_addrlen)) {
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
            addrstr,
            sizeof addrstr);

        std::cout << "Client: connected to " << addrstr << std::endl;

        freeaddrinfo(servinfo);
    }

    void *recieve()
    {
        int numbytes;
        char buf[MAXDATASIZE];
        if(-1 == (numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0))) {
            std::cerr << "recv() error: " << std::strerror(errno) << std::endl; 
            return (int*)3; // <<-- this looks terrible
        }

        std::cout << "Client: recieved " << buf << std::endl;

        return buf;
    }

    void *send()
    {
        // ima ded
        // fix just a week away
    }

    void end()
    {
        close(sockfd);  // <<-- something feels off, need a check
    }
};
}

#endif