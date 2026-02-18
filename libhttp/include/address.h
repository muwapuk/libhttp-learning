#ifndef ADDRESS_H
#define ADDRESS_H
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>

namespace libhttp
{
struct Address
{
    // Constructor with default hints
    Address(std::string addr, std::string port): addrstr(addr), portstr(port)
    {
        Address(addr, port, NULL);
    }
    Address(std::string addr, std::string port, addrinfo *hints): addrstr(addr), portstr(port)
    {
        if(0 != getaddrinfo(addrstr.c_str(),
                    portstr.c_str(),
                    hints,
                    &servinfo))
        {
            std::cerr << "getaddrinfo() error: " << std::strerror(errno) << std::endl; 
            throw 1;
        }
    }

    std::string addrstr;
    std::string portstr;
    addrinfo *servinfo;

    void *get_in_addr(sockaddr *sa)
    {
        if(sa->sa_family == AF_INET)
        { 
            return &(((sockaddr_in*)sa)->sin_addr);
        }
        return &(((sockaddr_in6*)sa)->sin6_addr);
    }
    int get_addr_info(std::string addr)    // i forgor what we wanted here :/
    {
        if(0 != getaddrinfo(addr.c_str(),
                    portstr.c_str(),
                    NULL,
                    &servinfo))
        {
            std::cerr << "get_addr_info() error: " << std::strerror(errno) << std::endl; 
            throw 1;
        }
    }
    // General method to generate new socket from the current address
    int get_socket()
    {
        int sockfd;
        if(-1 == (sockfd = socket(servinfo->ai_family,
                    servinfo->ai_socktype,
                    servinfo->ai_protocol)))
        {
            std::cerr << "socket() error: " << std::strerror(errno) << std::endl; 
            throw 1;
        }
        return sockfd;
    }
    void free_addr_info()
    {
        freeaddrinfo(servinfo);
    }
};
}

#endif