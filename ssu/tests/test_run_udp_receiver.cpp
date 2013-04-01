//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "link.h"

int main()
{
	try
	{
		ssu::link_host_state state;
		ssu::endpoint local_ep(boost::asio::ip::udp::v4(), 9660);
		boost::asio::io_service io_service;
		ssu::udp_link l(io_service, local_ep, state);
		l.send(local_ep, "\0SSUohai!", 10);
		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}
