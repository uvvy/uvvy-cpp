#include "link.h"

int main()
{
	try
	{
		ssu::link_host_state state;
		ssu::endpoint local_ep(boost::asio::ip::udp::v4(), 9660);
		boost::asio::io_service io_service;
		ssu::udp_link l(io_service, local_ep, state);
		l.send(local_ep, "\3\2\1\0hi!", 8);
		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}
