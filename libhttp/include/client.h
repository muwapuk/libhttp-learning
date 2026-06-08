#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <memory>
#include <netdb.h>
#include <optional>
#include <sys/socket.h>
#include <unistd.h>
#include <http.h>

namespace libhttp
{
class Client
{
	const int MAX_DATA_PAYLOAD = 32 * 1024;
    int sockfd;

	bool create_client_(const std::string &addr, int port);

    std::optional<Response> receive_response_();
    bool send_(int sockfd, const std::string &str);
    std::optional<Response> GetImpl_(const std::string& path, const Headers& headers, const std::string& body);
public:
    Client(const std::string addr);
    Client(const std::string addr, int port);
	Client(const Client &) = delete;
	Client& operator=(const Client&) = delete;
	~Client();

	std::optional<Response> Get(const std::string& path);
    std::optional<Response> Get(const std::string& path, const Headers& headers = {}); 
// Not implemented
	std::optional<Response> Post(std::string& path); 
	std::optional<Response> Put(std::string& path); 
	std::optional<Response> Patch(std::string& path); 
	std::optional<Response> Delete(std::string& path);
    std::optional<Response> Option(std::string& path);
};
}

#endif
