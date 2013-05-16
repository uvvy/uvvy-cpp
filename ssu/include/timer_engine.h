#pragma once

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace ssu {
namespace async {

class timer;

class timer_engine
{
	timer* origin_{0};
public:
	typedef boost::posix_time::time_duration duration_type;

	timer_engine(timer* t) : origin_(t) {}

	virtual void start(duration_type interval) = 0;
	virtual void stop() = 0;

protected:
	void timeout();
};

}
}
