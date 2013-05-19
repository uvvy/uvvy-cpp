#include "timer.h"
#include "timer_engine.h"

namespace ssu {
namespace async {

const timer::duration_type timer::retry_min = boost::posix_time::milliseconds(500);
const timer::duration_type timer::retry_max = boost::posix_time::minutes(1);
const timer::duration_type timer::fail_max  = boost::posix_time::seconds(20);

//=========================================================
// timer
//=========================================================

static timer::duration_type backoff(timer::duration_type interval, timer::duration_type max_interval = timer::fail_max)
{
	return std::min(interval * 3 / 2, max_interval);
}

timer::timer(ssu::timer_host_state* host)
{
	engine_ = host->create_timer_engine_for(this);
}

void timer::start(duration_type interval)
{
	interval_ = interval;
	active_ = true;
	engine_->start(interval_);
}

void timer::stop()
{
	engine_->stop();
	active_ = false;
}

void timer::restart()
{
	interval_ = backoff(interval_);
	start(interval_);
}

bool timer::has_failed() const
{
	return false;
}

//=========================================================
// default_timer_engine
//=========================================================

class default_timer_engine : public timer_engine
{
	boost::asio::deadline_timer interval_timer;

public:
	default_timer_engine(timer* t, boost::asio::io_service& io_service);
	~default_timer_engine();

	virtual void start(duration_type interval) override;
	virtual void stop() override;
};

default_timer_engine::default_timer_engine(timer* t, boost::asio::io_service& io_service)
	: timer_engine(t)
	, interval_timer(io_service)
{
}

default_timer_engine::~default_timer_engine()
{
	stop();
}

void default_timer_engine::start(duration_type interval)
{
	interval_timer.expires_from_now(interval);
	interval_timer.async_wait(std::bind(&default_timer_engine::timeout, this));
}

void default_timer_engine::stop()
{
}

void timer_engine::timeout()
{
	origin_->timeout(false);
}

} // namespace async

async::timer_engine* timer_host_state::create_timer_engine_for(async::timer* t)
{
	return new async::default_timer_engine(t, io_service);
}

} // namespace ssu
