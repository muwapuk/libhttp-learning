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
#include <unordered_set>
#include <vector>
#include <functional>

#include "http.h"

namespace libhttp 
{

enum StatusCode {
    OK_200 = 200,

    BadRequest_400 = 400,
    NotFound_404 = 404,
    MethodNotAllowed_405 = 405,
    PayloadTooLarge_413 = 413,
    URITooLong_414 = 414,
    RequestHeaderFieldsTooLarge_431 = 431,

    InternalServerError_500 = 500,
    NotImplemented_501 = 501,
    HTTPVersionNotSupported_505 = 505,
};
inline const char *status_messages(int status) {
    switch(status) {
        case OK_200: return "OK";

        case BadRequest_400: return "Bad Request";
        case NotFound_404: return "Not Found";
        case MethodNotAllowed_405: return "Method Not Allowed";
        case PayloadTooLarge_413: return "Payload Too Large";
        case URITooLong_414: return "URI Too Long";
        case RequestHeaderFieldsTooLarge_431: return "Request Header Fields Too Large";
        case NotImplemented_501: return "Not Implemented";
        case HTTPVersionNotSupported_505: return "HTTP Version Not Supported";
        default:
        case InternalServerError_500: return "Internal Server Error";
    }
}

struct Connection {
    int client_fd;
    std::string ip_str;
    
    std::string write_buffer;
    std::string read_buffer;

    Request request;

    //bool keep_alive = false;
};

class Server
{	
	const int MAX_DATA_PAYLOAD = 32 * 1024;

    int listen_sockfd_;
    std::string ip_;
    std::string hostname_;
    int port_;

	using Handler = std::function<void(const Request &, Response &)>;
	using Handlers = std::unordered_map<std::string, Handler>;
	using ErrorHandlers = std::unordered_map<int, Handler>;
    Handlers get_handlers_;
    Handlers post_handlers_;
    Handlers put_handlers_;
    Handlers patch_handlers_;
    Handlers delete_handlers_;
    Handlers options_handlers_;
    ErrorHandlers error_handlers_;

	int poll_descriptors_count_;
	std::vector<pollfd> poll_descriptors_;
    std::unordered_map<int, Connection> connections_;
    std::unordered_set<int> pending_closes_;
public:
    bool listen(const std::string &host, int port);

	void Get(std::string path, Handler); 
// Not implemented
	void Post(std::string path, Handler); 
	void Put(std::string path, Handler); 
	void Patch(std::string path, Handler); 
	void Delete(std::string path, Handler);
    void Option(std::string path, Handler);
    void Error(StatusCode status, Handler);
//
private:
    // Returns socket fd
    int create_socket_(const std::string &host,
                        int port,
			   		    int socket_flags = 0);    
	void fill_server_info_(const std::string &host);
	void process_descriptors_(int fd_count);
	void handle_new_connection_();
    void close_connection_(int client_fd);
	void handle_client_data_(int pollfd_index);
	void handle_request_(Connection&);
    Handlers& choose_handlers_(std::string& method);
    Response make_error_response(StatusCode code);
    void handle_bad_request_(Connection&);
	bool send_(int client_fd, const std::string&);
    void close_connection(int client_fd);
};

}

#endif
