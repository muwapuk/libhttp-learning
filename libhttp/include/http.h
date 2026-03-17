#ifndef HTTP_H
#define HTTP_H
#include <string>
#include <unordered_map>
#include <memory>

namespace libhttp {

const int MAX_METHOD_SIZE = 16;
const int MAX_PATH_SIZE = 2048;
const int MAX_VERSION_SIZE = 16;
const int MAX_REASON_SIZE = 2048;
const int STATUS_CODE_STR_SIZE = 3;
const int MAX_HEADERS_SIZE = 4'194'304;

struct Request {
	std::string method = std::string(MAX_METHOD_SIZE, 0); // GET
	std::string path = std::string(MAX_PATH_SIZE, 0);   // /path
	std::string version = std::string(MAX_VERSION_SIZE, 0); 	// HTTP/1.1

	std::unordered_map<std::string, std::string> headers;

	bool parse_string(const std::string &req);
private:
	enum ReadState {
		METHOD,
		PATH,
		VERSION	
	};
	size_t index = 0;
	bool parse_first_line(const std::string &req);
};

struct Response {
	std::string version = std::string(MAX_VERSION_SIZE, 0);
	int status_code;
	std::string reason = std::string(MAX_REASON_SIZE, 0);

	std::unordered_map<std::string, std::string> headers;

	std::string body;

	bool parse_string(const std::string &resp);
private:
	enum ReadState {
		VERSION,
		STATUS,
		REASON	
	};
	size_t index = 0;
	bool parse_first_line(const std::string &resp);
};

}
#endif
