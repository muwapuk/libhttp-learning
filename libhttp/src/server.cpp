#include "server.h"

#include "http.h"
#include "networkfuncs.h"
#include "logger.h"

#include <cassert>
#include <cstring>
#include <string.h>

using namespace libhttp;

bool Server::listen(const std::string &host, int port)
{
	listen_sockfd = create_socket(host, port);
	if(listen_sockfd < 0) {
        logError("Could not create socket!");
		return 1;
	}
	fill_server_info(host);

	if(::listen(listen_sockfd, 10) == -1) {
        logError("Could not set socket listen!");
        logDebug(std::string("listen() error: ") + std::strerror(errno));
		return 1;
	}
    logInfo(std::string("Listening on: ") + ip + ':' + std::to_string(port));
	
    poll_descriptors.push_back(
        pollfd {
            .fd = listen_sockfd,
            .events = POLLIN,
        }
    );

	for(;;) {
		int poll_count = poll(poll_descriptors.data(), poll_descriptors.size(), -1);
		if(poll_count == -1) {
            logError("Could not poll socket events!");
            logDebug(std::string("poll() error: ") + std::strerror(errno));
			break;
		}
		process_descriptors(poll_count);
	}
	return 0;
}
int Server::create_socket(const std::string &host,
						  int port,
                          int socket_flags)
{
    int listen_sockfd;
	addrinfo hints;

	memset(&hints, 0, sizeof hints);
	hints.ai_socktype = SOCK_STREAM; // TCP

	addrinfo *servinfo;
    auto gai_result = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &servinfo);
    if(gai_result != 0) {
        logDebug(std::string("getaddrinfo() error: ") + gai_strerror(gai_result));
		return -1;
	}
	if(-1 == (listen_sockfd = socket(servinfo->ai_family,
							  servinfo->ai_socktype,
							  servinfo->ai_protocol))) {
        logDebug(std::string("socket() error: ")  + std::strerror(errno));
		return -1;
	}
	// Reuse port
	int yes=1;
	if(-1 == setsockopt(listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) { 
        logDebug(std::string("setsockopt() error: ")  + std::strerror(errno));
		return -1;
	}
	if(-1 == bind(listen_sockfd, servinfo->ai_addr, servinfo->ai_addrlen)) {
        logError(std::string("bind() error: ")  + std::strerror(errno));
		return -1;
	}
	freeaddrinfo(servinfo);

	return listen_sockfd;
}

void Server::fill_server_info(const std::string &host)
{
	sockaddr_storage addr;
	socklen_t addr_len = sizeof(addr);
	::getsockname(listen_sockfd, (sockaddr*)&addr, &addr_len);

	hostname = host;
	ip = get_ip_string((sockaddr*)&addr);
}
void Server::process_descriptors(int fd_count)
{
    int events_processed { 0 };
	for(int i = 0; i < poll_descriptors.size(); i++) {
		if(poll_descriptors[i].revents & (POLLIN | POLLHUP)) {
            events_processed++;
			if(poll_descriptors[i].fd == listen_sockfd)
				handle_new_connection();
			else
				handle_client_data(i);
		}
        if(events_processed == fd_count) break;
	}
}
void Server::handle_new_connection()
{
	sockaddr_storage client_addr;
	socklen_t client_addr_size = sizeof(client_addr);
	int client_fd = accept(listen_sockfd,
								 (sockaddr*)&client_addr,
								 &client_addr_size);
	if(client_fd == -1) {
        logError("Could not handle new connection");
        logDebug(std::string("accept() error: ") + std::strerror(errno));
		/* ERROR HANDING */
		return;
	}
    logInfo(std::string("Accepted new connection from: ") + get_ip_string((sockaddr*)&client_addr));
    logDebug(std::string("client_sock = ") + std::to_string(client_fd));

	poll_descriptors.push_back(
        pollfd {
            .fd = client_fd,
            .events = POLLIN,
            .revents = 0 
        }
    );
    connections[client_fd].sockfd = client_fd;
    connections[client_fd].ip_str = get_ip_string(client_fd);
}
// pollfd_index is decreased by 1 if descriptor removed from poll_descriptors
// Needed for correct iteration inside poll descriptors loop
void Server::handle_client_data(int& pollfd_index) 
{
	char buf[MAX_DATA_PAYLOAD];
	int client_fd = poll_descriptors[pollfd_index].fd; 

	int nbytes = recv(client_fd, buf, sizeof buf, 0); 

	if(nbytes <= 0) {
		if(nbytes == 0) {
            logInfo(std::string("Closed connection with ") + connections[client_fd].ip_str);
            logDebug(std::string("client_sock = ") + std::to_string(client_fd));
		} else {
            logDebug(std::string("recv: ") + std::strerror(errno));
		}
		close(client_fd);
		poll_descriptors.erase(poll_descriptors.begin() + pollfd_index--);
        connections.erase(client_fd);
	} else {
        connections[client_fd].read_buffer.append(buf, nbytes);

        auto& con { connections[client_fd] };

        auto result = con.request->parse_string(con.read_buffer);
        switch(result) {
            case ParseResult::Complete:
                logInfo("Got request from " + con.ip_str); 
                con.read_buffer.clear();
			    handle_request(client_fd, *con.request);
                // TODO keep alive or close
                break;
            case ParseResult::Incomplete:
                logInfo("Message from " + con.ip_str + " is not finished. Waiting for completion.");
                break;
            case ParseResult::Error:
                logInfo("Bad request from " + con.ip_str);
                // TODO analize buffer and close the connection
                break;
        }
        
	}
}
bool Server::send(int sockfd, const std::string &str)
{
	if(sockfd < 0) {
        logError("Bad socket for send!");
		return 1;
	}
	int strsize = str.length();
	int	nbytes = ::send(sockfd, str.data(), strsize, 0);
	if(nbytes == -1)
        logDebug(std::string("send() error: ") + std::strerror(errno));
	return 0;
}
void Server::handle_request(int clientsock, Request &req) {
	if(!services.contains(req.path)) {
        logInfo("Request to unknown service from " 
                + connections[clientsock].ip_str 
                + ": " 
                + req.path);
		return;	
	}
	Response resp;
    auto& handler = services[req.path];
	handler(req, resp);
	
	// -----
	// SEND RESPONSE MADE BY HANDLER
	// -----
}

void Server::Get(std::string path, Handler handler)
{
	services[path] = handler;
}
