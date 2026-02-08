#ifndef CLIENT_H
#define CLIENT_H
#include <netdb.h>
#include <string>

namespace libhttp
{
struct Adress
{
    Adress(std::string addr, std::string port): addrstr(addr), portstr(port)
    {
        getaddrinfo(addrstr.c_str(), portstr.c_str(), NULL, NULL);
    }

    std::string addrstr;
    std::string portstr;
    addrinfo *servinfo;

    void *get_in_addr(sockaddr *sa)
    {
        if(sa->sa_family == AF_INET) { 
        return &(((sockaddr_in*)sa)->sin_addr);
        }
        return &(((sockaddr_in6*)sa)->sin6_addr);
    }
    int get_addr_info(std::string addr);
};
class Client
{
    int sockfd;

    int connect()
    {

    }
};
}

#endif