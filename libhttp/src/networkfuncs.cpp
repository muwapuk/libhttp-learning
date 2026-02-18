#include "networkfuncs.h"

void *get_in_addr(sockaddr *sa)
{
    if(sa->sa_family == AF_INET) { 
	return &(((sockaddr_in*)sa)->sin_addr);
    }
    return &(((sockaddr_in6*)sa)->sin6_addr);
}
std::string get_ip_string(const sockaddr *sa)
{
	std::string addrstr;
	
	if(sa->sa_family == AF_INET) {
		char addr_c_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &((sockaddr_in*)sa)->sin_addr, addr_c_str, INET_ADDRSTRLEN);
		return addr_c_str;
	} else if (sa->sa_family == AF_INET6) {
		char addr_c_str[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &((sockaddr_in*)sa)->sin_addr, addr_c_str, INET6_ADDRSTRLEN);
		return addr_c_str;
	} else {
		std::cerr << "get_ip_string: Unsupported address family.";
		return "";
	}
}

