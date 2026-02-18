#ifndef NETWORKFUNCS_H
#define NETWORKFUNCS_H

#include <netdb.h>
#include <string>
#include <arpa/inet.h>
#include <iostream>

void *get_in_addr(sockaddr *sa);
std::string get_ip_string(const sockaddr *sa);

#endif
