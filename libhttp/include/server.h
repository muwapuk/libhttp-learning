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
#include <functional>

#include "http.h"

namespace libhttp 
{

class Server
{	
public:
	using Handler = std::function<void(const Request &, Response &)>;
    bool listen(const std::string &host, int port);

	void Get(std::string path, Handler); 
// Not implemented
	void Post(std::string path, Handler); 
	void Put(std::string path, Handler); 
	void Patch(std::string path, Handler); 
	void Delete(std::string path, Handler);
//
private:
	const int MAX_DATA_PAYLOAD = 100 * 1024*1024;

    int listen_sockfd;
    std::string ip;
    std::string hostname;
    int port;

	std::unordered_map<std::string, Handler>services;

	int poll_descriptors_count;
	size_t poll_descriptors_size = 4;
	std::vector<pollfd> poll_descriptors;

    // Returns socket fd
    int create_socket(const std::string &host,
                        int port,
			   		    int socket_flags = 0);    
	bool fill_socket_info(const std::string &host);
	void process_descriptors(int fd_count);
	void handle_new_connection();
	void add_to_poll_descriptors(int fd);
	void handle_client_data(int pollfd_index);
	void handle_request(int clientsock, Request &);
	bool send(int clientsock, const std::string&);
};

}

#endif
