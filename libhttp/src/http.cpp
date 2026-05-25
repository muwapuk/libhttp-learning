#include "http.h"

#include "logger.h"

#include <iostream>
#include <fstream>


using namespace libhttp;

namespace {
	ParseResult parse_headers_(const std::string &data, 
                               std::unordered_map<std::string, std::string> &headers,
                               int &pos);
	ParseResult parse_body_(const std::string &data, 
                            std::unordered_map<std::string, std::string> &headers,
                            std::string &body,
                            int &pos);
	bool is_valid_header_name_(const std::string &name);
	bool is_valid_header_value_(const std::string &val);
}

ParseResult Request::parse_string(const std::string &req)
{
    switch(parse_state_) {
        ParseResult res;
        case ParseState::StartLine:
             res = parse_first_line_(req);
            if(res == ParseResult::Incomplete) {
                return res;
            } else if (res == ParseResult::Error) {
                parse_state_ = ParseState::Error;
                return res;
            }
            parse_state_ = ParseState::Headers;
        case ParseState::Headers:
            res = parse_headers_(req, headers, parse_index_);
            if(res == ParseResult::Incomplete) {
                return res;
            } else if (res == ParseResult::Error) {
                parse_state_ = ParseState::Error;
                return res;
            }
            parse_state_ = ParseState::Body;
        case ParseState::Body:
            res = parse_body_(req, headers, body, parse_index_);
            if(res == ParseResult::Incomplete) {
                return res;
            } else if (res == ParseResult::Error) {
                parse_state_ = ParseState::Error;
                return res;
            }
            parse_state_ = ParseState::Complete;
        case ParseState::Complete:
            return ParseResult::Complete;
        case ParseState::Error:
            return ParseResult::Error;
    }
}
ParseResult Request::parse_first_line_(const std::string &req)
{
    auto firstLineEnd = req.find("\r\n");
    if(firstLineEnd == std::string::npos) 
        return ParseResult::Incomplete;
    std::string firstLine = req.substr(0, firstLineEnd);

    // METHOD
    auto tokenEnd = firstLine.find(' ');  
    if(tokenEnd == std::string::npos) { 
        logDebug("Invalid request first line!");
        return ParseResult::Error;
    }
    method = firstLine.substr(0, tokenEnd); 
    if(method.size() > MAX_METHOD_SIZE || method.size() <= 0) {
        logDebug("Invalid method size!");
        return ParseResult::Error;
    }
    firstLine.erase(0, tokenEnd);

    // PATH
    tokenEnd = firstLine.find(' ');  
    if(tokenEnd == std::string::npos) { 
        logDebug("Invalid request first line!");
        return ParseResult::Error;
    }
    path = firstLine.substr(0, tokenEnd); 
    if(path.size() > MAX_PATH_SIZE || path.size() <= 0) {
        logDebug("Invalid path size!");
        return ParseResult::Error;
    }
    firstLine.erase(0, tokenEnd);

    // VERSION
    version = firstLine; 
    if(version.size() > MAX_VERSION_SIZE || version.size() <= 0) {
        logDebug("Invalid version size!");
        return ParseResult::Error;
    }
    parse_index_ = firstLineEnd+2;
    return ParseResult::Complete;
}
ParseResult Response::parse_string(const std::string &resp)
{
    switch(parse_state_) {
        ParseResult res;
        case ParseState::StartLine:
             res = parse_first_line_(resp);
            if(res == ParseResult::Incomplete) {
                return res;
            } else if (res == ParseResult::Error) {
                parse_state_ = ParseState::Error;
                return res;
            }
            parse_state_ = ParseState::Headers;
        case ParseState::Headers:
            res = parse_headers_(resp, headers, parse_index_);
            if(res == ParseResult::Incomplete) {
                return res;
            } else if (res == ParseResult::Error) {
                parse_state_ = ParseState::Error;
                return res;
            }
            parse_state_ = ParseState::Body;
        case ParseState::Body:
            res = parse_body_(resp, headers, body, parse_index_);
            if(res == ParseResult::Incomplete) {
                return res;
            } else if (res == ParseResult::Error) {
                parse_state_ = ParseState::Error;
                return res;
            }
            parse_state_ = ParseState::Complete;
        case ParseState::Complete:
            return ParseResult::Complete;
        case ParseState::Error:
            return ParseResult::Error;
    }
}
ParseResult Response::parse_first_line_(const std::string &resp)
{
    auto firstLineEnd = resp.find("\r\n");
    if(firstLineEnd == std::string::npos) 
        return ParseResult::Incomplete;
    std::string firstLine = resp.substr(0, firstLineEnd);

    // VERSION
    auto tokenEnd = firstLine.find(' ');  
    if(tokenEnd == std::string::npos) { 
        logDebug("Invalid request first line!");
        return ParseResult::Error;
    }
    version = firstLine.substr(0, tokenEnd); 
    if(version.size() > MAX_VERSION_SIZE || version.size() <= 0) {
        logDebug("Invalid version size!");
        return ParseResult::Error;
    }
    firstLine.erase(0, tokenEnd);

    // STATUS CODE
    tokenEnd = firstLine.find(' ');  
    if(tokenEnd == std::string::npos) { 
        logDebug("Invalid request first line!");
        return ParseResult::Error;
    }
    std::string statusCodeStr = firstLine.substr(0, tokenEnd); 
    if(statusCodeStr.size() > STATUS_CODE_STR_SIZE || statusCodeStr.size() <= 0) {
        logDebug("Invalid status code size!");
        return ParseResult::Error;
    }
    try {
        status_code = std::stoi(statusCodeStr);
    } catch (const std::exception& e) {
        logDebug("Invalid status code!");
        return ParseResult::Error;
    }
    firstLine.erase(0, tokenEnd);

    // REASON
    version = firstLine; 
    if(version.size() > MAX_REASON_SIZE || version.size() <= 0) {
        logDebug("Invalid reason size!");
        return ParseResult::Error;
    }
    parse_index_ = firstLineEnd+2;
    return ParseResult::Complete;
}
namespace {
bool is_valid_header_name_(const std::string &name)
{
	return name.find_first_not_of(ALLOWED_HEADER_NAME_CHARS) == std::string::npos;
}
bool is_valid_header_value_(const std::string &val)
{
	return val.find_first_not_of(ALLOWED_HEADER_VALUE_CHARS) == std::string::npos;
}
ParseResult parse_headers_(const std::string &data, 
				    std::unordered_map<std::string, std::string> &headers,
                    int& pos)
{
    auto headersEnd = data.find("\r\n\r\n");
    if(headersEnd == std::string::npos)
        return ParseResult::Incomplete;
    std::string headersStr = data.substr(pos, headersEnd);
    if(headersStr.size() > MAX_HEADERS_SIZE) {
        logDebug("Headers size too large!");
        return ParseResult::Error;
    }

    std::string token;
    for(;;) {
        token = headersStr.substr(0, headersStr.find("\r\n"));
        if(token.empty()) // found "\r\n\r\n"
            break;
        auto delimiterPos = token.find(' ');
        if(delimiterPos == std::string::npos) {
            logDebug("Invalid header!");
            return ParseResult::Error;
        }
        std::string name { token.substr(0,delimiterPos) };
        std::string value { token.substr(delimiterPos+1, token.size()-delimiterPos) };
        if(is_valid_header_name_(name)) {
            logDebug("Invalid header name!");
            return ParseResult::Error;
        }
        if(is_valid_header_value_(value)) {
            logDebug("Invalid header value!");
            return ParseResult::Error;
        }
        headers[name] = value;
    }
    pos = headersEnd+4;
    return ParseResult::Complete;
}

ParseResult parse_body_(const std::string &data, 
                        std::unordered_map<std::string, std::string> &headers,
                        std::string &body,
                        int &pos)
{
    if(headers["Transfer-Encoding"] == "Chunked") {
        logInfo("Unsupported body format!");
        return ParseResult::Error;
    }
    if(!headers["Content-Length"].empty()) {
        int contentLen;
        try {
            contentLen = std::stoi(headers["Content-Length"]);
        } catch (const std::exception& e) {
            logDebug("Invalid Content-Length!");
            return ParseResult::Error;
        }
        if(contentLen > MAX_BODY_SIZE) {
            logInfo("Content-Length too large!");
            return ParseResult::Error;
        }
        if(contentLen > data.size()-pos) {
            body.append(data.begin()+pos, data.end());
            pos = data.size();
            return ParseResult::Incomplete;
        } else {
            body.append(data.begin()+pos, data.begin()+pos+contentLen);
            pos = pos+contentLen;
            return ParseResult::Complete;
        }
    }
    return ParseResult::Complete;
}
} // namespace end

bool Response::set_header(const std::string &key, const std::string &val)
{
	if(key == "") {
        logDebug("Header name cannot be empty");
		return 1;
	}
	if(!is_valid_header_name_(key)) {
        logDebug("Invalid header name");
		return 1;
	}
	if(!is_valid_header_value_(val)) {
        logDebug("Invalid header value");
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
        logError("Failed to open content file for reading!");
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
        logError("Failed to open content file for reading!");
	} else {
		std::string content{std::istreambuf_iterator<char>(content_file), std::istreambuf_iterator<char>()};
		body.assign(content);	
		content_file.close();
	}
}

bool Request::set_header(const std::string &key, const std::string &val)
{
	if(key == "") {
        logDebug("Header name cannot be empty");
		return 1;
	}
	if(!is_valid_header_name_(key)) {
        logDebug("Invalid header name");
		return 1;
	}
	if(!is_valid_header_value_(val)) {
        logDebug("Invalid header value");
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
        logError("Failed to open content file for reading!");
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
        logError("Failed to open content file for reading!");
	} else {
		std::string content{std::istreambuf_iterator<char>(content_file), std::istreambuf_iterator<char>()};
		body.assign(content);	
		content_file.close();
	}
}
