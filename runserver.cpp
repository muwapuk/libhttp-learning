#include "server.h"

int main(int argc, char **argv)
{
	libhttp::Server srv;

	srv.listen("127.0.0.1", 25566);

	return 0;	
}
