#ifndef HTTP_H
#define HTTP_H
#include <string>
#include <unordered_map>

namespace libhttp {

const int MAX_PATH_SIZE = 2048;
const int MAX_REASON_SIZE = 2048;
const int STATUS_CODE_STR_SIZE = 3;
const int MAX_HEADERS_SIZE = 8*1024;
const int MAX_BODY_SIZE = 100*1024*1024;

const std::string ALLOWED_HEADER_NAME_CHARS = "!#$%&'*+-.^_`|~abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"; 
const std::string ALLOWED_HEADER_VALUE_CHARS= "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

enum class ParseState {
    Error,
    StartLine,
    Headers,
    Body,
    Complete,
};
enum class ParseResult {
    Error,
    Incomplete,
    Complete,
};

struct Request {
	std::string method; // GET
	std::string path;  // /path
	std::string version; 	// HTTP/1.1

	std::unordered_map<std::string, std::string> headers;

	size_t content_length;
	std::string body;

	ParseResult parse_string(const std::string &req);
    std::string serialize();

	bool set_header(const std::string &key, const std::string &val);
	void set_content(const char *s, size_t n, const std::string &content_type);
	void set_content(const std::string &s, const std::string &content_type);
	void set_content(std::string &&s, const std::string &content_type);

	void set_file_content(const std::string &path,
    					  const std::string &content_type);
  	void set_file_content(const std::string &path);
private:
	int parse_index_ = 0;
    ParseState parse_state_ = ParseState::StartLine;
	ParseResult parse_first_line_(const std::string &req);
};

struct Response {
	std::string version;
	int status_code;
	std::string reason;

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
	
	ParseResult parse_string(const std::string &resp);
    std::string serialize();
private:
	int parse_index_ = 0;
    ParseState parse_state_ = ParseState::StartLine;
	ParseResult parse_first_line_(const std::string &resp);
};

}
#endif
