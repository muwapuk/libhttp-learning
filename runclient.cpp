#include <string>
#include "client.h"


int main()
{
	libhttp::Client cl("127.0.0.1:25566");

	std::string str(22, '#');

	return 0;
}
