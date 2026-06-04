#include "server.h"

#include "http.h"
#include "networkfuncs.h"
#include "logger.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string.h>

using namespace libhttp;

bool Server::listen(const std::string &host, int port)
{
	listen_sockfd_ = create_socket_(host, port);
	if(listen_sockfd_ < 0) {
        logError("Could not create socket!");
		return 1;
	}
	fill_server_info_(host);

	if(::listen(listen_sockfd_, 10) == -1) {
        logError("Could not set socket listen!");
        logDebug(std::string("listen() error: ") + std::strerror(errno));
		return 1;
	}
    logInfo(std::string("Listening on: ") + ip_ + ':' + std::to_string(port));
	
    poll_descriptors_.push_back(
        pollfd {
            .fd = listen_sockfd_,
            .events = POLLIN,
        }
    );

	for(;;) {
		int poll_count = poll(poll_descriptors_.data(), poll_descriptors_.size(), -1);
		if(poll_count == -1) {
            logError("Could not poll socket events!");
            logDebug(std::string("poll() error: ") + std::strerror(errno));
			break;
		}
		process_descriptors_(poll_count);

        for(auto connection : pending_closes_) {
            logInfo("Closed connection with " + std::to_string(connection));
            close_connection(connection);
        }
        pending_closes_.clear();
	}
	return 0;
}
int Server::create_socket_(const std::string &host,
						  int port,
                          int socket_flags)
{
    int listen_sockfd_;
	addrinfo hints;

	memset(&hints, 0, sizeof hints);
	hints.ai_socktype = SOCK_STREAM; // TCP

	addrinfo *servinfo;
    auto gai_result = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &servinfo);
    if(gai_result != 0) {
        logDebug(std::string("getaddrinfo() error: ") + gai_strerror(gai_result));
		return -1;
	}
	if(-1 == (listen_sockfd_ = socket(servinfo->ai_family,
							  servinfo->ai_socktype,
							  servinfo->ai_protocol))) {
        logDebug(std::string("socket() error: ")  + std::strerror(errno));
		return -1;
	}
	// Reuse port
	int yes=1;
	if(-1 == setsockopt(listen_sockfd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes)) { 
        logDebug(std::string("setsockopt() error: ")  + std::strerror(errno));
		return -1;
	}
	if(-1 == bind(listen_sockfd_, servinfo->ai_addr, servinfo->ai_addrlen)) {
        logError(std::string("bind() error: ")  + std::strerror(errno));
		return -1;
	}
	freeaddrinfo(servinfo);

	return listen_sockfd_;
}

void Server::fill_server_info_(const std::string &host)
{
	sockaddr_storage addr;
	socklen_t addr_len = sizeof(addr);
	::getsockname(listen_sockfd_, (sockaddr*)&addr, &addr_len);

	hostname_ = host;
	ip_ = get_ip_string((sockaddr*)&addr);
}
void Server::process_descriptors_(int fd_count)
{
    int events_processed { 0 };
	for(int i = 0; i < poll_descriptors_.size(); i++) {
		if(poll_descriptors_[i].revents & (POLLIN | POLLHUP)) {
            events_processed++;
			if(poll_descriptors_[i].fd == listen_sockfd_)
				handle_new_connection_();
			else
				handle_client_data_(i);
		}
        if(events_processed == fd_count) break;
	}
}
void Server::handle_new_connection_()
{
	sockaddr_storage client_addr;
	socklen_t client_addr_size = sizeof(client_addr);
	int client_fd = accept(listen_sockfd_,
								 (sockaddr*)&client_addr,
								 &client_addr_size);
	if(client_fd == -1) {
        logError("Could not handle new connection");
        logDebug(std::string("accept() error: ") + std::strerror(errno));
		/* ERROR HANDING */
		return;
	}
    logInfo(std::string("Accepted new connection from: ") + get_ip_string((sockaddr*)&client_addr));
    logDebug(std::string("client_fd = ") + std::to_string(client_fd));
	poll_descriptors_.push_back(
        pollfd {
            .fd = client_fd,
            .events = POLLIN,
            .revents = 0 
        }
    );
    connections_[client_fd].client_fd = client_fd;
    connections_[client_fd].ip_str = get_ip_string(client_fd);
}
// pollfd_index is decreased by 1 if descriptor removed from poll_descriptors
// Needed for correct iteration inside poll descriptors loop
void Server::handle_client_data_(int pollfd_index) 
{
	char buf[MAX_DATA_PAYLOAD];
	int client_fd = poll_descriptors_[pollfd_index].fd; 

	int nbytes = recv(client_fd, buf, sizeof buf, 0); 

	if(nbytes <= 0) {
		if(nbytes == 0) {
            logInfo("Pending to close connection with " + connections_[client_fd].ip_str);
            logDebug(std::string("client_fd = ") + std::to_string(client_fd));
		} else {
            logDebug(std::string("recv: ") + std::strerror(errno));
		}
        pending_closes_.insert(client_fd);
	} else {
        connections_[client_fd].read_buffer.append(buf, nbytes);

        auto& con { connections_[client_fd] };

        auto result = con.request.parse_string(con.read_buffer);
        switch(result) {
            case ParseResult::Complete:
                logInfo("Got request from " + con.ip_str); 
                con.read_buffer.clear();
			    handle_request_(con);
                con.request = {};
                break;
            case ParseResult::Incomplete:
                logInfo("Message from " + con.ip_str + " is not finished. Waiting for completion.");
                break;
            case ParseResult::Error:
                logInfo("Bad request from " + con.ip_str);
                handle_bad_request_(con);
                break;
        }
        
	}
}
bool Server::send_(int sockfd, const std::string &str)
{
	if(sockfd < 0) {
        logError("Bad socket for send!");
		return 1;
	}
	int strsize = str.length();

    size_t total = 0;
    while(total < str.size()) {
        ssize_t n = ::send(sockfd,
                           str.data() + total,
                           str.size() - total,
                           0);
        if(n < 0) {
            logDebug(std::string("send() error: ") + std::strerror(errno));
            return false;
        }
        total += n;
    }
    return true;
}
Server::Handlers& Server::choose_handlers_(std::string& method) 
{
    if(method == "GET")
        return get_handlers_;
    if(method == "POST")
        return post_handlers_;
    if(method == "PUT")
        return put_handlers_;
    if(method == "PATCH")
        return patch_handlers_;
    if(method == "DELETE")
        return delete_handlers_;
    else //(method == "OPTIONS")
        return options_handlers_;
}
void Server::handle_request_(Connection& con) {
	Response resp;
	if(!choose_handlers_(con.request.method).contains(con.request.path)) {
        logInfo("Request to unknown service from " 
                + con.ip_str 
                + ": " 
                + con.request.path);
        resp.version = "HTTP/1.1";
        resp.status_code = NotFound_404;
        resp.reason = status_messages(NotFound_404);
        resp.body = "<h1>404 Not Found</h1>";
        resp.headers["Content-Length"] = std::to_string(resp.body.size());
        resp.headers["Connection"] = "close";
	} else {
        auto handler = choose_handlers_(con.request.method)[con.request.path];
        handler(con.request, resp);
    }
	// keep-alive
    bool keep_alive; 
    if(resp.headers.contains("Connection")) {
        keep_alive = resp.headers["Connection"] == "keep-alive";
    } else if(con.request.headers.contains("Connection")) {
        keep_alive = con.request.headers["Connection"] == "keep-alive";
    } else {
        keep_alive = con.request.version == "HTTP/1.1";
    }
    resp.headers["Connection"] = keep_alive ? "keep-alive" : "close";
    
    std::string response_string { resp.serialize() };
    if(response_string.empty()) { 
        logError("Could not build response string!");
        // TODO Internal server error
        return;
    }
    logInfo("Sending response...");
    send_(con.client_fd, response_string);

    if(!keep_alive) {
        logInfo("Pending to close connection with " + con.ip_str);
        pending_closes_.insert(con.client_fd);
    }
}
void Server::handle_bad_request_(Connection& con) 
{
	Response resp;
    if(error_handlers_.contains(BadRequest_400)) {
        error_handlers_[BadRequest_400](con.request, resp);
    } else {
        logInfo("Bad request from " 
                + con.ip_str);
        resp.version = "HTTP/1.1";
        resp.status_code = BadRequest_400;
        resp.reason = status_messages(BadRequest_400);
        resp.body = "<h1>400 Not Found</h1>";
        resp.headers["Connection"] = "close";
    }
    std::string response_string { resp.serialize() };
    if(response_string.empty()) { 
        logError("Could not build response string!");
        // TODO Internal server error
        return;
    }
    logInfo("Sending response...");
    send_(con.client_fd, response_string);

    logInfo("Pending to close connection with " + con.ip_str);
    pending_closes_.insert(con.client_fd);
}
void Server::close_connection(int client_fd)
{
    auto it = std::find_if(
        poll_descriptors_.begin(), poll_descriptors_.end(),
        [client_fd](const pollfd& p) {
            return p.fd == client_fd;
        }
    );
    if(it != poll_descriptors_.end())
        poll_descriptors_.erase(it);
    connections_.erase(client_fd);
    close(client_fd);
}
void Server::Get(std::string path, Handler handler)
{
	get_handlers_[path] = handler;
}
void Server::Post(std::string path, Handler handler)
{
	post_handlers_[path] = handler;
}
void Server::Put(std::string path, Handler handler)
{
    put_handlers_[path] = handler;
}
void Server::Patch(std::string path, Handler handler)
{
	patch_handlers_[path] = handler;
}
void Server::Delete(std::string path, Handler handler)
{
	delete_handlers_[path] = handler;
}
void Server::Option(std::string path, Handler handler)
{
	options_handlers_[path] = handler;
}
void Server::Error(StatusCode status, Handler handler)
{
    error_handlers_[status] = handler;
}
