//
// Part of Metta OS. Check http://metta.exquance.com for latest version.
//
// Copyright 2007 - 2013, Stanislav Karchebnyy <berkus@exquance.com>
//
// Distributed under the Boost Software License, Version 1.0.
// (See file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>

class debug
{
public:
    static std::mutex m;
    debug() {
        m.lock();
        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
        std::clog << "[D] " << boost::posix_time::to_iso_extended_string(now) << " T#" << std::this_thread::get_id() << ' ';
    }
    ~debug() { std::clog << std::endl; m.unlock(); }
    template <typename T>
    std::ostream& operator << (const T& v) { std::clog << v; return std::clog; }
};
