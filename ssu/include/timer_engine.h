#pragma once

namespace ssu {
namespace async {

class timer_engine
{
	timer* origin_{0};
public:
	timer_engine(timer* t) : origin_(t) {}

	virtual void start(duration_type interval) = 0;
	virtual void stop() = 0;

protected:
	void timeout();
};

}
}
