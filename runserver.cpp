#include "server.h"
#include <iostream>

#include <logger.h>

int main(int argc, char **argv)
{
	libhttp::Server srv;

	srv.Get("/", [](const libhttp::Request &req, libhttp::Response &res) {
		for(auto header : req.headers) {
			std::cout << header.first << ':' << header.second << '\n';
		}
		std::cout << req.body << std::endl;
	});

	srv.listen("127.0.0.1", 25566);

	return 0;	
}
