#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>

class debug
{
public:
	static std::mutex m;
	debug() {
		m.lock();
		boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
		std::clog << "[D] " << boost::posix_time::to_simple_string(now).c_str() << " T#" << std::this_thread::get_id() << ' ';
	}
	~debug() { std::clog << std::endl; m.unlock(); }
	template <typename T>
	std::ostream& operator << (const T& v) { std::clog << v; return std::clog; }
};
