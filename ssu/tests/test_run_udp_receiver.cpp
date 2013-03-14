#include "link.h"

int main()
{
	try
	{
		ssu::link_host_state state;
		boost::asio::io_service io_service;
		ssu::udp_link l(io_service, ssu::endpoint(boost::asio::ip::udp::v4(), 9660), state);
		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}
