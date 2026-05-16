#include <iostream>

const int& getNum()
{
	static const int id {0};

	return id;
}

int main()
{
	const int& a{getNum()};
	const int& b{getNum()};

	std::cout << a << " " << b << std::endl;
	return 0;
}
