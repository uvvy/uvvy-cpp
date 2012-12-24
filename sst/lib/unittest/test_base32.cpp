#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test_base32
#include <boost/test/unit_test.hpp>

#include "base32.h"
  
BOOST_AUTO_TEST_CASE(tobase32_1)
{
	QByteArray source1("hello!");
    BOOST_CHECK(Encode::toBase32(source1) == "something");
}