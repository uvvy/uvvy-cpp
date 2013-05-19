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
		ssu::udp_link l(local_ep, state);
		l.send(local_ep, "\0SSUohai!", 10);
		state.run_io_service();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}
