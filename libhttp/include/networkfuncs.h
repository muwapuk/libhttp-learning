#ifndef NETWORKFUNCS_H
#define NETWORKFUNCS_H

#include <netdb.h>
#include <string>
#include <arpa/inet.h>

void *get_in_addr(sockaddr *sa);
std::string get_ip_string(int sockfd);
std::string get_ip_string(const sockaddr *sa);
bool send(int sodkfd, const std::string&);

#endif
