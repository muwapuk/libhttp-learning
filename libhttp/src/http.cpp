#include "http.h"

#include "logger.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <set>


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
            [[fallthrough]];
        case ParseState::Headers:
            res = parse_headers_(req, headers, parse_index_);
            if(res == ParseResult::Incomplete) {
                return res;
            } else if (res == ParseResult::Error) {
                parse_state_ = ParseState::Error;
                return res;
            }
            parse_state_ = ParseState::Body;
            [[fallthrough]];
        case ParseState::Body:
            res = parse_body_(req, headers, body, parse_index_);
            if(res == ParseResult::Incomplete) {
                return res;
            } else if (res == ParseResult::Error) {
                parse_state_ = ParseState::Error;
                return res;
            }
            parse_state_ = ParseState::Complete;
            [[fallthrough]];
        case ParseState::Complete:
            return ParseResult::Complete;
        case ParseState::Error:
            return ParseResult::Error;
        default:
            return ParseResult::Error;
    }
}
std::string Request::serialize() 
{
    const std::set<std::string> methods{
        "GET",     "HEAD",    "POST",  "PUT",   "DELETE",
        "CONNECT", "OPTIONS", "TRACE", "PATCH", "PRI"};
    if (methods.find(method) == methods.end()) {
        logError("Serialize error: Request struct is invalid!");
        return "";
    }
    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        logError("Serialize error: Invalid HTTP version!");
        return "";
    }
    
    if(path.empty() || path.size() > MAX_PATH_SIZE) {
        logError("Serialize error: Request path is invalid!");
        return "";
    }
    // First line
    headers["Content-Length"] = std::to_string(body.size());
    std::string reqstr = method + ' ' + path + ' ' + version + "\r\n";
    // Headers
    for(auto& header : headers) {
        reqstr += header.first + ": " + header.second + "\r\n";
    }
    reqstr += "\r\n";
    // Body
    reqstr += body;

    return reqstr; 
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
    const std::set<std::string> methods{
        "GET",     "HEAD",    "POST",  "PUT",   "DELETE",
        "CONNECT", "OPTIONS", "TRACE", "PATCH", "PRI"};

    if (methods.find(method) == methods.end()) {
        logInfo("Invalid HTTP method!");
        return ParseResult::Error;
    }
    firstLine.erase(0, tokenEnd+1);

    // PATH
    tokenEnd = firstLine.find(' ');  
    if(tokenEnd == std::string::npos) { 
        logDebug("Invalid request first line!");
        return ParseResult::Error;
    }
    path = firstLine.substr(0, tokenEnd); 
    if(path.size() > MAX_PATH_SIZE) {
        logDebug("Invalid path size!");
        return ParseResult::Error;
    }
    firstLine.erase(0, tokenEnd+1);

    // VERSION
    version = firstLine; 
    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        logInfo("Invalid HTTP version!");
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
            [[fallthrough]];
        case ParseState::Headers:
            res = parse_headers_(resp, headers, parse_index_);
            if(res == ParseResult::Incomplete) {
                return res;
            } else if (res == ParseResult::Error) {
                parse_state_ = ParseState::Error;
                return res;
            }
            parse_state_ = ParseState::Body;
            [[fallthrough]];
        case ParseState::Body:
            res = parse_body_(resp, headers, body, parse_index_);
            if(res == ParseResult::Incomplete) {
                return res;
            } else if (res == ParseResult::Error) {
                parse_state_ = ParseState::Error;
                return res;
            }
            parse_state_ = ParseState::Complete;
            [[fallthrough]];
        case ParseState::Complete:
            return ParseResult::Complete;
        case ParseState::Error:
            return ParseResult::Error;
        default:
            return ParseResult::Error;
    }
}

std::string Response::serialize() 
{
    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        logError("Serialize error: Invalid HTTP version!");
        return "";
    }
    if(status_code < 100 || status_code > 999) {
        logError("Serialize error: Status code is invalid!");
        return "";
    }
    // First line
    std::string respstr = version + ' ' + std::to_string(status_code) + ' ' + reason + "\r\n";
    // Headers
    if(!headers.contains("Content-Length"))
        headers["Content-Length"] = std::to_string(body.size());
    for(auto& header : headers) {
        respstr += header.first + ": " + header.second + "\r\n";
    }
    respstr += "\r\n";
    // Body
    respstr += body;

    return respstr; 
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
    version = firstLine; 
    if (version != "HTTP/1.1" && version != "HTTP/1.0") {
        logInfo("Invalid HTTP method!");
        return ParseResult::Error;
    }
    firstLine.erase(0, tokenEnd+1);

    // STATUS CODE
    tokenEnd = firstLine.find(' ');  
    if(tokenEnd == std::string::npos) { 
        logDebug("Invalid request first line!");
        return ParseResult::Error;
    }
    std::string statusCodeStr = firstLine.substr(0, tokenEnd); 
    if(statusCodeStr.size() != STATUS_CODE_STR_SIZE) {
        logDebug("Invalid status code size!");
        return ParseResult::Error;
    }
    try {
        status_code = std::stoi(statusCodeStr);
    } catch (const std::exception& e) {
        logDebug("Invalid status code!");
        return ParseResult::Error;
    }
    firstLine.erase(0, tokenEnd+1);

    // REASON
    reason = firstLine; 
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
    std::string headersStr = data.substr(pos, headersEnd - pos + 4);
    if(headersStr.size() > MAX_HEADERS_SIZE) {
        logDebug("Headers size too large!");
        return ParseResult::Error;
    }

    std::string token;
    for(;;) {
        token = headersStr.substr(0, headersStr.find("\r\n"));
        headersStr.erase(0, token.length()+2);
        if(token.empty()) // found "\r\n\r\n"
            break;
        auto delimiterPos = token.find(':');
        if(delimiterPos == std::string::npos) {
            logDebug("Invalid header!");
            return ParseResult::Error;
        }
        std::string name { token.substr(0,delimiterPos) };
        std::string value { token.substr(delimiterPos+1) };
        if(!value.empty() && value[0] == ' ')
            value.erase(0, 1);
        if(!is_valid_header_name_(name)) {
            logDebug("Invalid header name!");
            return ParseResult::Error;
        }
        if(!is_valid_header_value_(value)) {
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
    if(headers["Transfer-Encoding"] == "chunked") {
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
