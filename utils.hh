#ifndef UTILS_HH
#define UTILS_HH

#include <string>
#include <stdexcept>

void die(std::string msg)
{
	throw std::runtime_error(msg);
}

#endif


