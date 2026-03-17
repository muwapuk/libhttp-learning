#include "http.h"

#include <iostream>


using namespace libhttp;

bool parse_headers(const std::string &data, std::unordered_map<std::string, std::string> &headers);

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
	ReadState read_state = METHOD;
	std::cout << req << std::endl;	
	for(int i = 0; i < req.length(); i++) {
		switch(read_state) {
			case METHOD:
				if(req[i] == ' ' || index == MAX_METHOD_SIZE) {
					method[index] = '\0';
					read_state = PATH;	
					index = 0;
					continue;
				}
				method[index] = req[i];
			break;
			case PATH:
				if(req[i] == ' ' || index == MAX_PATH_SIZE) {
					path[index] = '\0';
					read_state = VERSION;	
					index = 0;
					continue;
				}
				path[index] = req[i];
			break;
			case VERSION:
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
	ReadState read_state = VERSION;
	std::string strstatus = std::string(STATUS_CODE_STR_SIZE, 0); // Status is a 3 digit string (200, 404) 
	for(int i = 0; i < resp.length(); i++) {
		switch(read_state) {
			case VERSION:
				if(resp[i] == ' ' || index == MAX_METHOD_SIZE) {
					version[index] = '\0';
					read_state = STATUS;	
					index = 0;
					continue;
				}
				version[index] = resp[i];
			break;
			case STATUS:
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
					read_state = VERSION;	
					index = 0;
					continue;
				}
				strstatus[index] = resp[i];
			break;
			case REASON:
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

bool parse_headers(const std::string &data, 
				   std::unordered_map<std::string, std::string> &headers)
{
	std::string name = "";
	std::string value = "";
	std::string header_name_allowed_characters = "!#$%&'*+-.^_`|~abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"; 
	std::string header_value_allowed_characters = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

	bool reading_name = 1; // If not -> reading value

	for(int i; i < data.length(); i++) {
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
			} else if(header_name_allowed_characters.find(data[i]) != std::string::npos) {
				name.push_back(data[i]);
			} else {
				std::cerr << "Request header name contains not allowed character!" << std::endl;	
				return 1;
			}
		} else { // Reading header value
			if(data[i] == '\n') {
				if(i+5 <= data.length()
				&& std::string{data[i+1],  data[i+2], data[i+3], data[i+4]} == "\r\n\r\n") {
					i += 4;
					goto headers_parse_exit;
				} 
				headers[name] = value;
				name = "";
				value = "";
				reading_name = true;		
			} else if(header_value_allowed_characters.find(data[i]) != std::string::npos) {
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
