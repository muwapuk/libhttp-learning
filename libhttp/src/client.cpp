#include "client.h"

#include "logger.h"

#include <cstring>
#include <regex>
#include <networkfuncs.h>

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

		if(hostname.empty() || port.empty())
			return;
	} else {
        logError("Invalid URL");
		return;
	}
	int portNum = std::stoi(port);

    create_client_(hostname, portNum);
}
Client::Client(std::string host, int port)
{
	create_client_(host, port);	
}
Client::~Client() 
{
	if(sockfd != -1) 
		close(sockfd);
}
bool Client::create_client_(const std::string &host, int port)
{
	addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *hostinfo;
    auto gai_result = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &hostinfo);
    if(gai_result != 0) {
        logDebug(std::string("getaddrinfo() error: ") + gai_strerror(gai_result));
		return 1;
    }
    addrinfo *p;
    char addrstr[INET6_ADDRSTRLEN];
    for(p = hostinfo; p != NULL; p=p->ai_next) { 
		if(-1 == (sockfd = socket(p->ai_family,
								  p->ai_socktype,
								  p->ai_protocol))) {
            logDebug(std::string("socket() error: ") + std::strerror(errno));
		    continue;
		}
		inet_ntop(p->ai_family,
			get_in_addr((sockaddr*)&p->ai_addr),
			addrstr, sizeof addrstr);
        logInfo(std::string("Attempting to connect to ") + addrstr);

		if(-1 == connect(sockfd, p->ai_addr, p->ai_addrlen)) {
            logDebug(std::string("connect() error: ") + std::strerror(errno));
		    close(sockfd);
		    continue;
    	}
		break;
    }
    if(p == NULL) {
        logError("Connection failed!");
		sockfd = -1;
		return 1;
    }
    logInfo(std::string("Connected to ") + addrstr);
    logDebug(std::string("sockfd = ") + std::to_string(sockfd));

    freeaddrinfo(hostinfo);
	return 0;
}
std::optional<Response> Client::receive_response_()
{
    std::string data;
	char buf[MAX_DATA_PAYLOAD];
	if(sockfd == -1) {
        logError("Bad socket for receive!");
		return {};
	}

    Response resp;
    ssize_t offset = 0;
    for(;;) {
        ssize_t nbytes = recv(sockfd, buf+offset, MAX_DATA_PAYLOAD-offset, 0);
        if(nbytes == 0) {
            if(resp.parse_string(data) == ParseResult::Complete)
                return resp;
            else 
                return std::nullopt;
        }
        if(nbytes < 0) {
           logError(std::string("recv() error ") + strerror(errno));
            return std::nullopt;
        }
        offset += nbytes;
        data.append(buf+offset, nbytes);

        auto result = resp.parse_string(data);
        switch(result) {
            case ParseResult::Complete:
                logInfo("Got response from " + get_ip_string(sockfd)); 
                return resp; 
            case ParseResult::Incomplete:
                logInfo("Message from " + get_ip_string(sockfd) + " is not finished. Waiting for completion.");
                break;
            case ParseResult::Error:
                logInfo("Bad response from " + get_ip_string(sockfd));
                return std::nullopt;
        }
    }
}
std::optional<Response> Client::GetImpl_(const std::string& path, const Headers& headers, const std::string& body)
{
	Request req;

    req.version = "HTTP/1.1";
    req.path = path;
    req.method = "GET";
    req.headers = headers;
    req.body = body;
    if(!req.body.empty())
        req.headers["Content-Length"] = std::to_string(body.length());

    if(!send_(sockfd, req.serialize())) { 
        logError("Could not send request!");
        return std::nullopt;
    }
    return receive_response_();
}
std::optional<Response> Client::Get(const std::string& path) 
{
    return GetImpl_(path, {}, "");
}
std::optional<Response> Client::Get(const std::string& path, const Headers& headers)
{
    return GetImpl_(path, headers, "");
}
bool Client::send_(int sockfd, const std::string &str)
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
