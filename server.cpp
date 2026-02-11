#include "server.h"


void *get_in_addr(sockaddr *sa)
{
    if(sa->sa_family == AF_INET) { 
	return &(((sockaddr_in*)sa)->sin_addr);
    }
    return &(((sockaddr_in6*)sa)->sin6_addr);
}
std::string get_ip_string(const sockaddr *sa)
{
	std::string addrstr;
	
	if(sa->sa_family == AF_INET) {
		char addr_c_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &((sockaddr_in*)sa)->sin_addr, addr_c_str, INET_ADDRSTRLEN);
		return addr_c_str;
	} else if (sa->sa_family == AF_INET6) {
		char addr_c_str[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &((sockaddr_in*)sa)->sin_addr, addr_c_str, INET6_ADDRSTRLEN);
		return addr_c_str;
	} else {
		std::cerr << "get_ip_string: Unsupported address family.";
		return "";
	}
}
int Server::create_socket(const std::string &host,
						  int port,
						  /*int address_family,*/ 
						  int socket_flags)
{
	int sockfd;
	addrinfo hints;

	memset(&hints, 0, sizeof hints);
	hints.ai_socktype = SOCK_STREAM; // TCP

	addrinfo *servinfo;
	std::string portstr = std::to_string(port);
	if(0 != getaddrinfo(host.c_str(), portstr.c_str(), &hints, &servinfo)) {
	    std::cerr << "getaddrinfo() error: " << std::strerror(errno) << std::endl; 
	    exit(1);
	}
	if(-1 == (sockfd = socket(servinfo->ai_family,
							  servinfo->ai_socktype,
							  servinfo->ai_protocol))) {
		    std::cerr << "socket() error: " << std::strerror(errno) << std::endl; 
	}
	// Reuse port
	int yes=1;
	if(-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) { 
	    std::cerr << "setsockoopt() error: " << std::strerror(errno) << std::endl; 
	}
	if(-1 == bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) {
	    std::cerr << "bind() error: " << std::strerror(errno) << std::endl; 
	}
	freeaddrinfo(servinfo);
	return sockfd;
}
bool Server::fill_socket_info(const std::string &host)
{
	sockaddr_storage addr;
	socklen_t addr_len = sizeof(addr);
	::getsockname(sockfd, (sockaddr*)&addr, &addr_len);

	hostname = host;
	ip = get_ip_string((sockaddr*)&addr);

	return 0;
}
bool Server::handle_connection(int client_socket)
{
	if(!fork()) {
	    close(sockfd);
	    if(send(client_socket, "Hello, world!", 13, 0) == -1)
		std::cerr << "Send: " << std::strerror(errno) << std::endl;
	    close(client_socket);
	    exit(0);
	}
	close(client_socket);
	return 0;
}
bool Server::listen(const std::string &host, int port)
{
	sockfd = create_socket(host, port);
	if(sockfd < 0) {
		std::cerr << "Bad socket" << std::endl;
		exit(1);
	}
	fill_socket_info(host);
	std::cout << "Listening on: " << ip << ':' << port << std::endl;

	if(::listen(sockfd, 10) == -1) {
		std::cerr << "listen: " << std::strerror(errno) << std::endl;
	}
	

	sockaddr_storage client_addr;
	socklen_t client_addr_size = sizeof(client_addr);
	for(;;) {
		int client_sock = accept(sockfd,
								 (sockaddr*)&client_addr,
								 &client_addr_size);
		if(client_sock == -1) {
		    std::cerr << "Connection accept: " << std::strerror(errno) << std::endl;
			exit(1);
		    continue;
		} else {
			std::cout << "Accepted connection: " << get_ip_string((sockaddr*)&client_addr) << std::endl;
		}
		handle_connection(client_sock);
	}
}
