#include <thread>
#include "logging.h"

void logger_threadfun()
{
	for (int i = 0; i < 10000; ++i)
	{
		debug() << "Testing string on a nonexistent socket" << 15 << 44.1 << 'z' << true; 
	}
}

int main()
{
	std::thread t1(logger_threadfun);
	std::thread t2(logger_threadfun);
	t1.join();
	t2.join();
}
