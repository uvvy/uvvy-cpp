#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test_ssu_link
#include <boost/test/unit_test.hpp>

#include "link.h"

BOOST_AUTO_TEST_CASE(created_link)
{
}

BOOST_AUTO_TEST_CASE(receive_too_small_packet)
{
	byte_array msg({'a', 'b', 'c'});
	ssu::link l(0);
	ssu::link_endpoint le;

	l.receive(msg, le);
}
