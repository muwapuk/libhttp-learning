#include "client.h"

#include "logger.h"

#include <cstring>
#include <regex>
#include <sys/socket.h>
#include <networkfuncs.h>

const char *PORT = "3490";
const int MAXDATASIZE = 100;
using namespace libhttp;

Client::Client(std::string url)
{
	// Match: Adrress
	// Group 1: Protocol (http, ftp, etc...)
	// Group 2: hostname/ipv4 (example.com, 192.168.0.1, ...)
	// Group 3: Port number 
	const static std::regex re(
    	R"((?:([a-z]+):\/\/)?(?:([^:\/?#]+))(?::(\d+))?)");
	std::smatch m;

	std::string hostname;
	std::string port;
	if(std::regex_match(url, m, re)) {
		hostname = m[2];
		port = m[3];

		if(hostname.empty() || port.empty())
			return;
	} else {
        logError("Invalid URL");
		return;
	}
	int portNum = std::stoi(port);

    create_client(hostname, portNum);
}
Client::Client(std::string host, int port)
{
	create_client(host, port);	
}
Client::~Client() 
{
	if(sockfd != -1) 
		close(sockfd);
}
bool Client::create_client(const std::string &host, int port)
{
	addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *hostinfo;
    if(0 != getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &hostinfo)) {
        logDebug(std::string("getaddrinfo() error: ") + std::strerror(errno));
		return 1;
    }
    addrinfo *p;
    char addrstr[INET6_ADDRSTRLEN];
    for(p = hostinfo; p != NULL; p=p->ai_next) { 
		if(-1 == (sockfd = socket(p->ai_family,
								  p->ai_socktype,
								  p->ai_protocol))) {
            logDebug(std::string("socket() error: ") + std::strerror(errno));
		    continue;
		}
		inet_ntop(p->ai_family,
			get_in_addr((sockaddr*)&p->ai_addr),
			addrstr, sizeof addrstr);
        logInfo(std::string("Attempting to connect to ") + addrstr);

		if(-1 == connect(sockfd, p->ai_addr, p->ai_addrlen)) {
            logDebug(std::string("connect() error: ") + std::strerror(errno));
		    close(sockfd);
		    continue;
    	}
		break;
    }
    if(p == NULL) {
        logError("Connection failed!");
		sockfd = -1;
		return 1;
    }
    logInfo(std::string("Connected to ") + addrstr);
    logDebug(std::string("sockfd = ") + std::to_string(sockfd));

    freeaddrinfo(hostinfo);
	return 0;
}
std::string Client::recieve()
{
	if(sockfd == -1) {
        logError("Bad socket for recieve!");
		return "";
	}
	std::string msg(MAXDATASIZE, 0);
	int nbytes = recv(sockfd, msg.data(), MAXDATASIZE, 0);

	return msg;
}
Response Client::Get(std::string path) 
{	
	Request req;
}
