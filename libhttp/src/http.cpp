#include "http.h"

#include <iostream>
#include <fstream>


using namespace libhttp;

namespace {
	bool parse_headers(const std::string &data, std::unordered_map<std::string, std::string> &headers);
	bool is_valid_header_name(const std::string &name);
	bool is_valid_header_value(const std::string &val);
}

bool Request::parse_string(const std::string &req)
{
	if(parse_first_line(req))
		return 1;
	if(parse_headers(req,headers))
		return 1;
	return 0;
}
bool Request::parse_first_line(const std::string &req)
{
	ReadState read_state = ReadState::method;
	std::cout << req << std::endl;	
	for(int i = 0; i < req.length(); i++) {
		switch(read_state) {
			case ReadState::method:
				if(req[i] == ' ' || index == MAX_METHOD_SIZE) {
					method[index] = '\0';
					read_state = ReadState::path;	
					index = 0;
					continue;
				}
				method[index] = req[i];
			break;
			case ReadState::path:
				if(req[i] == ' ' || index == MAX_PATH_SIZE) {
					path[index] = '\0';
					read_state = ReadState::version;	
					index = 0;
					continue;
				}
				path[index] = req[i];
			break;
			case ReadState::version:
				if(req[i] == '\n' || index == MAX_VERSION_SIZE) {
					version[index] = '\0';
					index = i; // Set index to first line end for headers parser
					goto request_parse_exit; 
				}
				version[index] = req[i];
			break;
			default: return 1; 
		}
	}
request_parse_exit:
	return 0;
}
bool Response::parse_string(const std::string &resp)
{
	if(parse_first_line(resp))
		return 1;
	if(parse_headers(resp,headers))
		return 1;
	return 0;
}
bool Response::parse_first_line(const std::string &resp)
{
	ReadState read_state = ReadState::version;
	std::string strstatus = std::string(STATUS_CODE_STR_SIZE, 0); // Status is a 3 digit string (200, 404) 
	for(int i = 0; i < resp.length(); i++) {
		switch(read_state) {
			case ReadState::version:
				if(resp[i] == ' ' || index == MAX_METHOD_SIZE) {
					version[index] = '\0';
					read_state = ReadState::status;	
					index = 0;
					continue;
				}
				version[index] = resp[i];
			break;
			case ReadState::status:
				if(resp[i] == ' ' || index == STATUS_CODE_STR_SIZE) {
					if(index != STATUS_CODE_STR_SIZE) {
						std::cerr << "Incorrect status code: " << strstatus << std::endl;
						return 1;
					}
					strstatus[index] = '\0';
					try {
						status_code = std::stoi(strstatus);
					} catch(const std::invalid_argument &e) {
						std::cerr << "Invalid status code: " << strstatus << std::endl;
					}
					read_state = ReadState::reason;	
					index = 0;
					continue;
				}
				strstatus[index] = resp[i];
			break;
			case ReadState::reason:
				if(resp[i] == '\n' || index == MAX_REASON_SIZE) {
					reason[index] = '\0';
					index = i; // Set index to first line end for headers parser
					goto response_parse_exit; 
				}
				reason[index] = resp[i];
			break;
			default: return 1; 
		}
	}
response_parse_exit:

	return 0;
}
namespace {
bool is_valid_header_name(const std::string &name)
{
	return name.find_first_not_of(ALLOWED_HEADER_NAME_CHARS) == std::string::npos;
}
bool is_valid_header_value(const std::string &val)
{
	return val.find_first_not_of(ALLOWED_HEADER_VALUE_CHARS) == std::string::npos;
}
bool parse_headers(const std::string &data, 
				   std::unordered_map<std::string, std::string> &headers)
{
	std::string name = "";
	std::string value = "";

	bool reading_name = 1; // If not -> reading value

	for(size_t i = 0; i < data.length(); i++) {
		if(i >= MAX_HEADERS_SIZE) {
			std::cerr << "Request headers too large!" << std::endl;
			return 1;
		}
		if(reading_name) {
			if(data[i] == ':') {
				if(name.empty()) {
					std::cerr << "Request header key empty!" << std::endl;
					return 1;
				}
				// Skip space character in header value
				if(i+1 < data.length())
					i++;
				reading_name = false;
			} else if(ALLOWED_HEADER_NAME_CHARS .find(data[i]) != std::string::npos) {
				name.push_back(data[i]);
			} else {
				std::cerr << "Request header name contains not allowed character!" << std::endl;	
				return 1;
			}
		} else { // Reading header value
			if(data[i] == '\n') {
				if(i+5 <= data.length()
				&& data.compare(i + 1, 4, "\r\n\r\n") == 0) {
					i += 4;
                    break;
				} 
				headers[name] = value;
				name = "";
				value = "";
				reading_name = true;		
			} else if(ALLOWED_HEADER_VALUE_CHARS.find(data[i]) != std::string::npos) {
				value.push_back(data[i]);
			} else {
				std::cerr << "Request header value contains not allowed character!" << std::endl;	
				return 1;
			}
		}
	}
headers_parse_exit:
	return 0;
}
}

bool Response::set_header(const std::string &key, const std::string &val)
{
	if(key == "") {
		std::cerr << "Header name cannot be empty!" << std::endl;
		return 1;
	}
	if(!is_valid_header_name(key)) {
		std::cerr << "Invalid header name!" << std::endl;
		return 1;
	}
	if(!is_valid_header_value(val)) {
		std::cerr << "Invalid header value!" << std::endl;
		return 1;
	}
	if(val.empty()) 
		headers.erase(key);
	else
		headers[key] = val;
	return 0;
}
void Response::set_content(const char *s, size_t n, const std::string &content_type) 
{
	body.assign(s, n);
	set_header("Content-Type", content_type);	
}
void Response::set_content(const std::string &s, const std::string &content_type)
{
	body = s;
	set_header("Content-Type", content_type);	
}
void Response::set_content(std::string &&s, const std::string &content_type)
{
	body = s;
	set_header("Content-Type", content_type);	
}
void Response::set_file_content(const std::string &path,
    					  const std::string &content_type)
{
	std::ifstream content_file(path, std::ios::in);
	if(!content_file.is_open()) {
		std::cerr << "Failed to open content file for reading!" << std::endl;
	} else {
		set_header("Content-Type", content_type);	
		std::string content{std::istreambuf_iterator<char>(content_file), std::istreambuf_iterator<char>()};
		body.assign(content);	
		content_file.close();
	}
}
void Response::set_file_content(const std::string &path)
{

	std::ifstream content_file(path, std::ios::in);
	if(!content_file.is_open()) {
		std::cerr << "Failed to open content file for reading!" << std::endl;
	} else {
		std::string content{std::istreambuf_iterator<char>(content_file), std::istreambuf_iterator<char>()};
		body.assign(content);	
		content_file.close();
	}
}

bool Request::set_header(const std::string &key, const std::string &val)
{
	if(key == "") {
		std::cerr << "Header name cannot be empty!" << std::endl;
		return 1;
	}
	if(!is_valid_header_name(key)) {
		std::cerr << "Invalid header name!" << std::endl;
		return 1;
	}
	if(!is_valid_header_value(val)) {
		std::cerr << "Invalid header value!" << std::endl;
		return 1;
	}
	if(val.empty()) 
		headers.erase(key);
	else
		headers[key] = val;
	return 0;
}
void Request::set_content(const char *s, size_t n, const std::string &content_type) 
{
	body.assign(s, n);
	set_header("Content-Type", content_type);	
}
void Request::set_content(const std::string &s, const std::string &content_type)
{
	body = s;
	set_header("Content-Type", content_type);	
}
void Request::set_content(std::string &&s, const std::string &content_type)
{
	body = s;
	set_header("Content-Type", content_type);	
}
void Request::set_file_content(const std::string &path,
    					  const std::string &content_type)
{
	std::ifstream content_file(path, std::ios::in);
	if(!content_file.is_open()) {
		std::cerr << "Failed to open content file for reading!" << std::endl;
	} else {
		set_header("Content-Type", content_type);	
		std::string content{std::istreambuf_iterator<char>(content_file), std::istreambuf_iterator<char>()};
		body.assign(content);	
		content_file.close();
	}
}
void Request::set_file_content(const std::string &path)
{

	std::ifstream content_file(path, std::ios::in);
	if(!content_file.is_open()) {
		std::cerr << "Failed to open content file for reading!" << std::endl;
	} else {
		std::string content{std::istreambuf_iterator<char>(content_file), std::istreambuf_iterator<char>()};
		body.assign(content);	
		content_file.close();
	}
}
