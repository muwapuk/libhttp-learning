#include "server.h"

int main(int argc, char **argv)
{
	Server srv;

	srv.listen("192.168.0.254", 3490);

	return 0;	
}
