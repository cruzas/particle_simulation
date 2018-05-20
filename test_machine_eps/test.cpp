#include <cmath>

#include <cstring>
#include <iostream>
#include <limits>

int 
main(int argc, char *argv[])
{
	std::cout << "machine_eps:" << std::numeric_limits<double>::epsilon() << "\n";
	std::cout << "Size of double: " << sizeof(double) << " bytes" << "\n";

	return 0;
}

