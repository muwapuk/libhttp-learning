#ifndef HTTP_H
#define HTTP_H
#include <string>
#include <unordered_map>

namespace libhttp {

const int MAX_METHOD_SIZE = 16;
const int MAX_PATH_SIZE = 2048;
const int MAX_VERSION_SIZE = 16;
const int MAX_REASON_SIZE = 2048;
const int STATUS_CODE_STR_SIZE = 3;
const int MAX_HEADERS_SIZE = 4'194'304;

const std::string ALLOWED_HEADER_NAME_CHARS = "!#$%&'*+-.^_`|~abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"; 
const std::string ALLOWED_HEADER_VALUE_CHARS= "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

enum class ParseResult {
    Ok,

};
struct Request {
	std::string method = std::string(MAX_METHOD_SIZE, 0); // GET
	std::string path = std::string(MAX_PATH_SIZE, 0);   // /path
	std::string version = std::string(MAX_VERSION_SIZE, 0); 	// HTTP/1.1

	std::unordered_map<std::string, std::string> headers;

	size_t content_length;
	std::string body;

	bool parse_string(const std::string &req);

	bool set_header(const std::string &key, const std::string &val);
	void set_content(const char *s, size_t n, const std::string &content_type);
	void set_content(const std::string &s, const std::string &content_type);
	void set_content(std::string &&s, const std::string &content_type);

	void set_file_content(const std::string &path,
    					  const std::string &content_type);
  	void set_file_content(const std::string &path);
private:
	enum class ReadState {
		method,
		path,
		version	
	};
	size_t index = 0;
	bool parse_first_line(const std::string &req);
};

struct Response {
	std::string version = std::string(MAX_VERSION_SIZE, 0);
	int status_code = -1;
	std::string reason = std::string(MAX_REASON_SIZE, 0);

	std::unordered_map<std::string, std::string> headers;

	size_t content_length;
	std::string body;

	bool set_header(const std::string &key, const std::string &val);
	void set_content(const char *s, size_t n, const std::string &content_type);
	void set_content(const std::string &s, const std::string &content_type);
	void set_content(std::string &&s, const std::string &content_type);

	void set_file_content(const std::string &path,
    					  const std::string &content_type);
  	void set_file_content(const std::string &path);
	
	bool parse_string(const std::string &resp);
private:
	enum class ReadState {
		version,
		status,
		reason	
	};
	size_t index = 0;
	bool parse_first_line(const std::string &resp);
};

}
#endif
