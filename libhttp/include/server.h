#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#include <poll.h>
#include <vector>
#include <map>

#include "http.h"

namespace libhttp 
{
class Server
{	
	const int MAX_CONNECTIONS = 15;

    int listen_sockfd;
    std::string ip;
    std::string hostname;
    int port;

	std::map<std::string, void(*)(int, const Request&)> services;

	int poll_descriptors_count;
	size_t poll_descriptors_size = 4;
	std::vector<pollfd> poll_descriptors;

    // Returns socket fd
    int create_socket(const std::string &host, int port,
			   		  /*int address_family,*/ int socket_flags = 0);    
	bool fill_socket_info(const std::string &host);
	void process_descriptors(int fd_count);
	void handle_new_connection();
	void handle_client_data(int pollfd_index);
	void add_to_poll_descriptors(int fd);
	bool send(int clientsock, const std::string&);

	bool add_service_handler(std::string path, void(*)(int, const Request&));
	bool handle_request(int clientsock, Request &);
public:
    bool listen(const std::string &host, int port);
};

}

#endif
