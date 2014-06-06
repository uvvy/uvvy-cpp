//
// Part of Metta OS. Check http://atta-metta.net for latest version.
//
// Copyright 2007 - 2014, Stanislav Karchebnyy <berkus@atta-metta.net>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#define BOOST_TEST_MODULE Test_jitterbuffer
#include <boost/test/unit_test.hpp>

#include "voicebox/jitterbuffer.h"

using namespace std;
using namespace voicebox;

BOOST_AUTO_TEST_CASE(create_jb)
{
    jitterbuffer jb_;
    BOOST_CHECK(conn != nullptr);
}

// TO TEST:
//
// Normal packet arrival
// Out-of-order packet arrival
// Too late packets
// Filling the gaps
// Maximum stream gap warning
// Catch up if too slow (old packet discard)

BOOST_AUTO_TEST_CASE(packets_in_order)
{
}

BOOST_AUTO_TEST_CASE(packets_out_of_order)
{
}

BOOST_AUTO_TEST_CASE(packets_too_late)
{
}

