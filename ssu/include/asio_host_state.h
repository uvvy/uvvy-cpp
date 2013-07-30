#pragma once

#include <boost/asio/io_service.hpp>

namespace ssu {

/**
 * Host state incapsulating asio run loop and possibly other related variables.
 */
class asio_host_state
{
protected:
	/**
	 * I/O service that needs to be run in order to service protocol interactions.
	 */
	boost::asio::io_service io_service;

public:
	/**
	 * A blocking call that runs service run loop that services all
	 * asynchronous interactions.
	 */
	inline void run_io_service() { io_service.run(); }
	/**
	 * Some components of the SSU stack need access to the I/O service.
	 */
	inline boost::asio::io_service& get_io_service() { return io_service; }
};

} // namespace ssu
