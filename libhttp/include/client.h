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

#include "networkfuncs.h"
namespace libhttp
{
class Client
{
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

    const int MAXDATASIZE = 100;
    int sockfd;

	int create_client(std::string addr, int port);
public:
    Client(const std::string addr);
    Client(const std::string addr, int port);
	Client(const Client &) = delete;
	Client& operator=(const Client&) = delete;
	~Client();

    //void *recieve()
    //{
    //    int numbytes;
    //    char buf[MAXDATASIZE];
    //    if(-1 == (numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0))) {
    //        std::cerr << "recv() error: " << std::strerror(errno) << std::endl; 
    //        return (int*)3; // <<-- this looks terrible
    //    }

    //    std::cout << "Client: recieved " << buf << std::endl;

    //    return buf;
    //}

    //void *send()
    //{
    //}
};
}

#endif
