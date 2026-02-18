#include "client.h"

#include <regex>

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

		std::cout << "Hostname is: " << hostname << '\n'
				  << "Port is: " << port << std::endl;
		if(hostname.empty() || port.empty())
			return;
	} else {
		std::cerr << "Bad URL" << std::endl;
		return;
	}
	int portNum = std::stoi(port);
	
	Client(hostname, portNum); 
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
int Client::create_client(std::string host, int port)
{
	addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *hostinfo;
    if(0 != getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &hostinfo)) {
		std::cerr << "getaddrinfo() error: " << std::strerror(errno) << std::endl; 
		return 1;
    }
    sockfd = socket(hostinfo->ai_family, hostinfo->ai_socktype, hostinfo->ai_protocol);
    if(-1 == (sockfd = socket(hostinfo->ai_family,
							  hostinfo->ai_socktype,
							  hostinfo->ai_protocol))) {
		std::cerr << "socket() error: " << std::strerror(errno) << std::endl; 
		return 1;
    }
    addrinfo *p;
    char addrstr[INET6_ADDRSTRLEN];
    for(p = hostinfo; p != NULL; p=p->ai_next) { 
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
		sockfd = -1;
		return 2;
    }
    inet_ntop(p->ai_family,
	      get_in_addr((sockaddr*)&p->ai_addr),
	      addrstr, sizeof addrstr);
    std::cout << "Client: connected to " << addrstr << std::endl;

    freeaddrinfo(hostinfo);
	return 0;
}
