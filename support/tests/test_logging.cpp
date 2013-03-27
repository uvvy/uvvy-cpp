//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <thread>
#include "logging.h"

void logger_threadfun()
{
	for (int i = 0; i < 10000; ++i)
	{
		logger::debug() << "Testing string on a nonexistent socket" << 15 << 44.1 << 'z' << true; 
		logger::info() << "Testing string on a nonexistent socket" << 15 << 44.1 << 'z' << true; 
		logger::warning() << "Testing string on a nonexistent socket" << 15 << 44.1 << 'z' << true; 
	}
}

int main()
{
	std::thread t1(logger_threadfun);
	std::thread t2(logger_threadfun);
	t1.join();
	t2.join();
}
