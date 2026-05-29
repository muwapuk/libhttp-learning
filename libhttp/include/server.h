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
#include <memory>

#include "http.h"

namespace libhttp 
{

struct Connection {
    int sockfd;
    std::string ip_str;
    
    std::string write_buffer;
    std::string read_buffer;

    std::unique_ptr<Request> request { new Request };

    bool keep_alive;
};

class Server
{	
	const int MAX_DATA_PAYLOAD = 32 * 1024;

    int listen_sockfd;
    std::string ip;
    std::string hostname;
    int port;

	using Handler = std::function<void(const Request &, Response &)>;
	std::unordered_map<std::string, Handler>services;

	int poll_descriptors_count;
	std::vector<pollfd> poll_descriptors;
    std::unordered_map<int, Connection> connections;

public:
    bool listen(const std::string &host, int port);

	void Get(std::string path, Handler); 
// Not implemented
	void Post(std::string path, Handler); 
	void Put(std::string path, Handler); 
	void Patch(std::string path, Handler); 
	void Delete(std::string path, Handler);
//
private:
    // Returns socket fd
    int create_socket(const std::string &host,
                        int port,
			   		    int socket_flags = 0);    
	void fill_server_info(const std::string &host);
	void process_descriptors(int fd_count);
	void handle_new_connection();
	void handle_client_data(int& pollfd_index);
	void handle_request(int clientsock, Request &);
	bool send(int clientsock, const std::string&);
};

}

#endif
