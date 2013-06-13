//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#define BOOST_TEST_MODULE Test_ssu_link
#include <boost/test/unit_test.hpp>

#include "link.h"

BOOST_AUTO_TEST_CASE(created_link)
{
    ssu::link_host_state host;
    ssu::link l(host);
}

BOOST_AUTO_TEST_CASE(receive_too_small_packet)
{
	byte_array msg({'a', 'b', 'c'});
	ssu::link_host_state host;
	ssu::link l(host);
	ssu::link_endpoint le;

	// l.receive(msg, le);
}
