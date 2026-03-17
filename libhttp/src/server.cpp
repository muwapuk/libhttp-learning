#include "server.h"

#include "http.h"
#include "networkfuncs.h"

#include <iostream>
#include <cstring>
#include <string.h>

using namespace libhttp;

bool Server::listen(const std::string &host, int port)
{
	listen_sockfd = create_socket(host, port);
	if(listen_sockfd < 0) {
		std::cerr << "Bad socket" << std::endl;
		return 1;
	}
	fill_socket_info(host);
	std::cout << "Listening on: " << ip << ':' << port << std::endl;

	if(::listen(listen_sockfd, 10) == -1) {
		std::cerr << "listen: " << std::strerror(errno) << std::endl;
		return 1;
	}
	
	poll_descriptors.reserve(poll_descriptors_size);
	pollfd listen_pollfd;
	listen_pollfd.fd = listen_sockfd;
	listen_pollfd.events = POLLIN;
	poll_descriptors.push_back(listen_pollfd);

	for(;;) {
		int poll_count = poll(poll_descriptors.data(), poll_descriptors.size(), -1);
		if(poll_count == -1) {
			std::cerr << "Poll: " << std::strerror(errno) << std::endl;
			/* ERROR HANDING */
			break;
		}
		process_descriptors(poll_count);
	}
	return 0;
}
int Server::create_socket(const std::string &host,
						  int port,
						  /*int address_family,*/ 
						  int socket_flags)
{
	int listen_sockfd;
	addrinfo hints;

	memset(&hints, 0, sizeof hints);
	hints.ai_socktype = SOCK_STREAM; // TCP

	addrinfo *servinfo;
	std::string portstr = std::to_string(port);
	if(0 != getaddrinfo(host.c_str(), portstr.c_str(), &hints, &servinfo)) {
	    std::cerr << "getaddrinfo() error: " << std::strerror(errno) << std::endl; 
		return -1;
	}
	if(-1 == (listen_sockfd = socket(servinfo->ai_family,
							  servinfo->ai_socktype,
							  servinfo->ai_protocol))) {
		std::cerr << "socket() error: " << std::strerror(errno) << std::endl; 
		return -1;
	}
	// Reuse port
	int yes=1;
	if(-1 == setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) { 
	    std::cerr << "setsockoopt() error: " << std::strerror(errno) << std::endl; 
		return -1;
	}
	if(-1 == bind(listen_sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) {
	    std::cerr << "bind() error: " << std::strerror(errno) << std::endl; 
		return -1;
	}
	freeaddrinfo(servinfo);

	return listen_sockfd;
}

bool Server::fill_socket_info(const std::string &host)
{
	sockaddr_storage addr;
	socklen_t addr_len = sizeof(addr);
	::getsockname(listen_sockfd, (sockaddr*)&addr, &addr_len);

	hostname = host;
	ip = get_ip_string((sockaddr*)&addr);

	return 0;
}
void Server::process_descriptors(int fd_count)
{
	for(int i = 0; i < fd_count; i++) {
		if(poll_descriptors[i].revents & (POLLIN | POLLHUP)) {
			if(poll_descriptors[i].fd == listen_sockfd)
				handle_new_connection();
			else
				handle_client_data(i);
		}
	}
}
void Server::handle_new_connection()
{
	sockaddr_storage client_addr;
	socklen_t client_addr_size = sizeof(client_addr);
	int client_sock = accept(listen_sockfd,
								 (sockaddr*)&client_addr,
								 &client_addr_size);
	if(client_sock == -1) {
	    std::cerr << "Connection accept: " << std::strerror(errno) << std::endl;
		/* ERROR HANDING */
		return;
	}
	std::cout << "Accepted connection on descriptor " << client_sock << ": " << get_ip_string((sockaddr*)&client_addr) << std::endl;
	add_to_poll_descriptors(client_sock);
}
void Server::add_to_poll_descriptors(int fd)
{
	pollfd newfd;
	newfd.fd = fd;
	newfd.events = POLLIN;
	newfd.revents = 0;

	poll_descriptors.push_back(newfd);
}
void Server::handle_client_data(int pollfd_index)
{
	char buf[999'999];
	int client_fd = poll_descriptors[pollfd_index].fd; 

	int nbytes = recv(client_fd, buf, sizeof buf, 0); 

	if(nbytes <= 0) {
		if(nbytes == 0) {
			std::cout << "Server: socket " << client_fd << " closed" << std::endl;
		} else {
			std::cerr << "recv: " << std::strerror(errno) << std::endl;
		}
		close(client_fd);
		poll_descriptors.erase(poll_descriptors.begin() + pollfd_index);
	} else {
		std::cout << "Server: recv message from fd " << client_fd << std::endl;	
		Request req;
		if(req.parse_string(buf)) {
			std::cerr << "Bad request from descriptor: " << client_fd << std::endl;
		} else {
			std::cout << req.method << ' ' << req.path << ' ' << req.version << '\n';
			for(auto header : req.headers) {
				std::cout << header.first << ": " << header.second << '\n';
			}
			std::fflush(stdout);	
		}
	}
}
bool Server::send(int sockfd, const std::string &str)
{
	if(sockfd == -1) {
		std::cerr << "Bad socket! Exiting..." << std::endl;
		return 1;
	}
	int strsize = str.length();
	int	nbytes = ::send(sockfd, str.data(), strsize, 0);
	if(nbytes == -1)
		std::cerr << "send(): " << std::strerror(errno) << std::endl;

	return 0;
}

bool Server::add_service_handler(std::string path, void(*handler)(int, const Request&)) {
	services[path] = handler;	
	return 0;
}
bool Server::handle_request(int clientsock, Request &req) {
	if(!services.count(req.path)) {
		return 1;	
	}
	services[req.path](clientsock, req);
	return 0;
}
