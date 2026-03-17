#include "client.h"
#include <string>

int main()
{
	libhttp::Client cl("127.0.0.1:25566");

	std::string str(22, '#');
	std::cout << str << std::endl << cl.recieve() << std::endl;

	return 0;
}
