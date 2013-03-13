#pragma once

#include <iostream>
#include <mutex>

class debug
{
public:
	static std::mutex m;
	debug() { m.lock(); }
	~debug() { std::clog << std::endl; m.unlock(); }
	template <typename T>
	std::ostream& operator << (const T& v) { std::clog << v; return std::clog; }
};
