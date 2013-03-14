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
#include <iomanip>
#include <mutex>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>

class logging
{
    static std::mutex m;
protected:
    logging() {
        m.lock();
	}
    virtual ~logging() { std::clog << std::endl; m.unlock(); }

public:
    template <typename T>
    std::ostream& operator << (const T& v) { std::clog << v; return std::clog; }
};

class debug : public logging
{
public:
    debug() : logging() {
        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
        std::clog << "[DEBUG] " << boost::posix_time::to_iso_extended_string(now) << " T#" << std::this_thread::get_id() << ' ';
    }
};

class info : public logging
{
public:
    info() : logging() {
        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
        std::clog << "[INFO ] " << boost::posix_time::to_iso_extended_string(now) << " T#" << std::this_thread::get_id() << ' ';
    }
};

class warning : public logging
{
public:
    warning() : logging() {
        boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
        std::clog << "[WARN ] " << boost::posix_time::to_iso_extended_string(now) << " T#" << std::this_thread::get_id() << ' ';
    }
};

/**
 * Helper to output a hexadecimal value with formatting to an iostream.
 * Usage: io << hex(value, 8, true, false)
 */
struct hex_output
{
    int ch;
    int width;
    bool fill;
    bool base;

    hex_output(int c, int w, bool f, bool b) : ch(c), width(w), fill(f), base(b) {}
};

inline std::ostream& operator<<(std::ostream& o, const hex_output& hs)
{
    return (o << std::setw(hs.width) << std::setfill(hs.fill ? '0' : ' ') << std::hex << (hs.base ? std::showbase : std::noshowbase) << hs.ch);
}

inline hex_output hex(int c, int w = 2, bool f = true, bool b = false)
{
    return hex_output(c,w,f,b);
}

