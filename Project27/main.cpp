#include <iostream>

int addnum(int a, int b) {
	if (a == b) {
		return 2*a;
	}

	return a + b;
}

int main(int argc, char* argv[])
{
	std::cout << "Hello, World!" << std::endl;
	auto result = 0;
	result = addnum(3, 4);
	return result;
}