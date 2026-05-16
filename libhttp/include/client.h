#ifndef CLIENT_H
#define CLIENT_H

#include <cerrno>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <http.h>

namespace libhttp
{
class Client
{
    const int MAXDATASIZE = 10;
    int sockfd;

	bool create_client(const std::string &addr, int port);

	std::string recieve();
public:
    Client(const std::string addr);
    Client(const std::string addr, int port);
	Client(const Client &) = delete;
	Client& operator=(const Client&) = delete;
	~Client();

	Response Get(std::string path);
};
}

#endif
